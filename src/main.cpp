#include "stdinc.h"
#include "parser.hpp"
#include "error.hpp"
#include "symtable.hpp"
#include "commands.hpp"
#include "compiler.hpp"
#include "codegen.hpp"

int main()
{
    _wchdir(L"../..");

    std::vector<shared_ptr<Script>> scripts;

    auto commands = get_test_commands();

    auto main = std::make_shared<Script>("test.sc", ScriptType::Main);
    auto symbols = SymTable::from_script(*main);
    symbols.apply_offset_to_vars(2);

    scripts.emplace_back(main);

    auto subdir = main->scan_subdir();

    auto ext_scripts = read_and_scan_symbols(subdir, symbols.extfiles.begin(), symbols.extfiles.end(), ScriptType::MainExtension);
    auto sub_scripts = read_and_scan_symbols(subdir, symbols.subscript.begin(), symbols.subscript.end(), ScriptType::Subscript);
    auto mission_scripts = read_and_scan_symbols(subdir, symbols.mission.begin(), symbols.mission.end(), ScriptType::Mission);

    for(auto& x : ext_scripts)
    {
        symbols.merge(std::move(x.second));
        scripts.emplace_back(x.first); // maybe move
    }

    for(auto& x : sub_scripts)
    {
        symbols.merge(std::move(x.second));
        scripts.emplace_back(x.first); // maybe move
    }

    for(auto& x : mission_scripts)
    {
        symbols.merge(std::move(x.second));
        scripts.emplace_back(x.first); // maybe move
    }

    for(auto& script : scripts)
    {
        script->annotate_tree(symbols, commands);
    }


    CompilerContext cc(main, symbols, commands);
    cc.compile();
    main->size = cc.compute_labels();

    Script::compute_script_offsets(scripts);
    symbols.compute_label_offsets_globally();
    
    CodeGenerator cgen(std::move(cc));
    cgen.generate();

    FILE* f = fopen("output.cs", "wb");
    fwrite(cgen.bytecode.get(), 1, cgen.script->size.value(), f);
    fclose(f);

    //getchar();
}

