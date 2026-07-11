#include "stdafx.h"

#include "ASTVisitor.h"

int wmain(int argc, wchar_t** argv)
{
    if (argc < 2)
    {
        std::wcout << L"Usage: OdrCop3 <folder to compile_commands.json> ";
        return -1;
    }

    std::filesystem::path jsonFolder = argv[1];
    std::error_code ec;
    if (!std::filesystem::exists(jsonFolder, ec) || !std::filesystem::is_directory(jsonFolder, ec))
    {
        std::wcerr << L"Path not found or not a directory: " << jsonFolder.wstring() << L'\n';
        return -1;
    }

    std::string error;
    auto compilations = clang::tooling::CompilationDatabase::loadFromDirectory(jsonFolder.string(), error);
    std::vector<std::string> files = compilations->getAllFiles();
    clang::tooling::ClangTool tool(*compilations, files);

    OdrCop3::AllMaps maps;
    OdrCop3::VisitorActionFactory factory(maps);
    tool.run(&factory);

    int violations  = 0;
        violations += OdrCop3::OdrViolationReporter::ReportOdrViolations(maps.    enumMap, std::cout);
        violations += OdrCop3::OdrViolationReporter::ReportOdrViolations(maps.functionMap, std::cout);
        violations += OdrCop3::OdrViolationReporter::ReportOdrViolations(maps. typedefMap, std::cout);
        violations += OdrCop3::OdrViolationReporter::ReportOdrViolations(maps.     udtMap, std::cout);
    if (violations == 0)
        std::wcout << L"No ODR violations found.\n";
    else
        std::wcout << violations << L" ODR violation(s) found.\n";
    return 0;
}
