#include <iomanip>
#include <sstream>

#include "pochivm/common.h"

#include "runtime_lib_builder/fake_symbol_resolver.h"
#include "runtime_lib_builder/reflective_stringify_parser.h"

#include "pochivm/ast_enums.h"
#include "metavar.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/IR/Module.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Operator.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Mangler.h"
#include "llvm/Support/DynamicLibrary.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/Orc/ExecutionUtils.h"
#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/TypeFinder.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Linker/Linker.h"

using namespace llvm;
using namespace llvm::orc;

namespace {

struct BoilerplateParam
{
    PochiVM::MetaVarType m_type;
    std::string m_name;
    std::string m_typename;
};

struct BoilerplateInstance
{
    std::vector<uint64_t> m_paramValues;
    void* m_ptr;
    std::string m_symbolName;
};

struct BoilerplatePack
{
    std::vector<BoilerplateParam> m_params;
    std::vector<BoilerplateInstance> m_instances;
};

static std::vector<std::pair<PochiVM::AstNodeType, BoilerplatePack>> g_allBoilerplatePacks;

}   // anonymous namespace

void __pochivm_register_fast_interp_boilerplate__(PochiVM::AstNodeType nodeType, PochiVM::MetaVarMaterializedList* list)
{
    BoilerplatePack p;
    for (const PochiVM::MetaVar& var : list->m_metavars)
    {
        BoilerplateParam bvar;
        bvar.m_type = var.m_type;
        bvar.m_name = std::string(var.m_name);
        if (bvar.m_type == PochiVM::MetaVarType::ENUM)
        {
            bvar.m_typename = ReflectiveStringifyParser::ParseTypeName(var.m_enum_typename);
        }
        else if (bvar.m_type == PochiVM::MetaVarType::BOOL)
        {
            bvar.m_typename = "bool";
        }
        else
        {
            ReleaseAssert(bvar.m_type == PochiVM::MetaVarType::PRIMITIVE_TYPE);
            bvar.m_typename = "TypeId";
        }
        p.m_params.push_back(bvar);
    }

    for (const PochiVM::MetaVarMaterializedInstance& inst : list->m_instances)
    {
        BoilerplateInstance binst;
        binst.m_paramValues = inst.m_values;
        binst.m_ptr = inst.m_fnPtr;
        binst.m_symbolName = "";
        p.m_instances.push_back(binst);
    }

    g_allBoilerplatePacks.push_back(std::make_pair(nodeType, p));
}

int main(int argc, char** argv)
{
    InitLLVM _init_llvm_(argc, argv);

    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();

    // params: [bc_file] [obj_file]

    ReleaseAssert(argc == 3);
    std::string bc_file = argv[1];
    std::string obj_file = argv[2];

    ReleaseAssert(bc_file.find(";") == std::string::npos);
    ReleaseAssert(obj_file.find(";") == std::string::npos);

    SMDiagnostic llvmErr;
    std::unique_ptr<LLVMContext> context(new LLVMContext);
    std::unique_ptr<Module> module = parseIRFile(bc_file, llvmErr, *context.get());

    if (module == nullptr)
    {
        fprintf(stderr, "[ERROR] An error occurred while parsing IR file '%s', detail:\n", bc_file.c_str());
        llvmErr.print(argv[0], errs());
        abort();
    }

    // Iterate the IR file and pick up all the function name symbols
    //
    std::set<std::string> allDefinitions, allDeclarations;
    for (Function& f : module->functions())
    {
        if (f.getLinkage() == GlobalValue::LinkageTypes::LinkOnceODRLinkage ||
            f.getLinkage() == GlobalValue::LinkageTypes::WeakODRLinkage ||
            f.getLinkage() == GlobalValue::LinkageTypes::ExternalLinkage ||
            f.getLinkage() == GlobalValue::LinkageTypes::ExternalWeakLinkage ||
            f.getLinkage() == GlobalValue::LinkageTypes::AvailableExternallyLinkage)
        {
            std::string name = f.getName();
            // Empty functions are declarations
            //
            if (!f.empty())
            {
                ReleaseAssert(!allDefinitions.count(name));
                allDefinitions.insert(name);
            }
            ReleaseAssert(!allDeclarations.count(name));
            allDeclarations.insert(name);
        }
    }

    // Beyond this point the IR module is gone: we move it to the JIT
    //
    {
        ExitOnError exitOnError;
        ThreadSafeModule tsm(std::move(module), std::move(context));

        // JIT execution function __pochivm_register_runtime_library__ to figure out
        // which functions are needed by the runtime library, and their detailed type information
        //
        std::unique_ptr<LLJIT> J = exitOnError(LLJITBuilder().create());

        char Prefix = EngineBuilder().selectTarget()->createDataLayout().getGlobalPrefix();
        std::unique_ptr<DynamicLibrarySearchGenerator> R = exitOnError(DynamicLibrarySearchGenerator::GetForCurrentProcess(Prefix));
        ReleaseAssert(R != nullptr);

        // For undefined symbol in the IR file, first try to resolve in host process (it
        // contains symbols from system library and our hook __pochivm_report_info__).
        // If failed, it should be a symbol implemented in other CPP files,
        // and we should only care about its address (and never actually need to invoke it),
        // so just resolve it to an unique address using our FakeSymbolResolver.
        //
        J->getMainJITDylib().addGenerator(std::move(R));
        AddFakeSymbolResolverGenerator(J.get());

        exitOnError(J->addIRModule(std::move(tsm)));

        auto entryPointSym = exitOnError(J->lookup("__pochivm_build_fast_interp_library__"));
        using _FnPrototype = void(*)(void);
        _FnPrototype entryPoint = reinterpret_cast<_FnPrototype>(entryPointSym.getAddress());
        entryPoint();

        // Match the function address with the symbol name
        //
        std::map<uintptr_t, std::string > addrToSymbol;
        std::set<uintptr_t> hasMultipleSymbolsMappingToThisAddress;

        for (const std::string& symbol : allDeclarations)
        {
            auto sym = exitOnError(J->lookup(symbol));
            uintptr_t addr = sym.getAddress();
            if (addrToSymbol.count(addr))
            {
                hasMultipleSymbolsMappingToThisAddress.insert(addr);
            }
            addrToSymbol[addr] = symbol;
        }

        for (auto it = g_allBoilerplatePacks.begin(); it != g_allBoilerplatePacks.end(); it++)
        {
            BoilerplatePack& bp = it->second;
            for (BoilerplateInstance& inst : bp.m_instances)
            {
                uintptr_t addr = reinterpret_cast<uintptr_t>(inst.m_ptr);
                if (hasMultipleSymbolsMappingToThisAddress.count(addr))
                {
                    fprintf(stderr, "[INTERNAL ERROR] A fastinterp boilerplate is resolved to an ambiguous address, "
                                    "in boilerplate for %s. Please report a bug.\n",
                            it->first.ToString());
                    abort();
                }
                ReleaseAssert(addrToSymbol.count(addr));
                inst.m_symbolName = addrToSymbol[addr];
                ReleaseAssert(allDefinitions.count(inst.m_symbolName));
            }
        }
    }

    for (auto it = g_allBoilerplatePacks.begin(); it != g_allBoilerplatePacks.end(); it++)
    {
        printf("%s:\n", it->first.ToString());
        BoilerplatePack& bp = it->second;
        for (BoilerplateInstance& inst : bp.m_instances)
        {
            printf("    %s\n", inst.m_symbolName.c_str());
        }
    }

    return 0;
}
