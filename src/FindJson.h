#pragma once

#include <vector>
#include <iostream>
#include <fstream>
#include <filesystem>

namespace OdrCop3
{
    struct Find
    {
        static std::vector<std::string> FullyQualifiedFileNames()
        {
            std::vector<std::string> fqfns;

            std::ifstream in(Find::JsonFile());
            std::string line;
            while (std::getline(in, line))
            {
                size_t pos = line.find("\"command\":");
                if (pos == std::string::npos)
                    continue;

                line = line.substr(pos+12);
                line = line.substr(0, line.size()-1); // trim off trailing '"'

                fqfns.push_back(Find::FQFNFromCommandLine(line));
            }
            return fqfns;
        }
        static std::string JsonFile()
        {
            std::filesystem::path json = L".\\compile_commands.json";

            std::error_code ec;
            if (false == std::filesystem::exists(json, ec) && !ec)
            {
                json = L"..\\compile_commands.json";
                if (false == std::filesystem::exists(json, ec) && !ec)
                {
                    json = L"..\\..\\compile_commands.json";
                    if (false == std::filesystem::exists(json, ec) && !ec)
                    {
                        json = L"..\\..\\..\\compile_commands.json";
                        if (false == std::filesystem::exists(json, ec) && !ec)
                            return "";
                    }
                }
            }
            return json.string();
        }
        static std::string FQFNFromCommandLine(const std::string& commandLine)
        {
            std::vector<std::string> args;
            {
                std::wstring commandLineW(commandLine.begin(), commandLine.end());
                int numArgs = 0;
                LPWSTR* splitArgs = CommandLineToArgvW(commandLineW.c_str(), &numArgs);
                std::vector<std::wstring> wargs;
                for(int i=0; i<numArgs; ++i)
                    wargs.push_back({splitArgs[i]});
                LocalFree(splitArgs);

                // narrow
                args.reserve(wargs.size());
                std::transform(wargs.begin(), wargs.end(), std::back_inserter(args),
                                        [](const std::wstring& ws)
                                        {
                                            std::string s;
                                            s.reserve(ws.size());
                                            std::transform(ws.begin(), ws.end(), std::back_inserter(s), [](wchar_t wc) { return static_cast<char>(wc); });
                                            return s;
                                        });
            }

            std::string fo, cppFile;
            for(size_t j=1; j<args.size(); ++j) // not range for, because I might need to increment j myself
            {
                const auto& arg = args[j];
                if (arg[0] == '/')
                {
                    if (arg.starts_with("/Fo"))
                    {
                        if (arg == "/Fo")
                        {   // the next arg is the actual path
                            fo = args[++j]; // This won't blow up because it would be malformed
                            continue;
                        }
                        // othersize it's the rest of this arg
                        fo = arg.substr(3);
                        continue;
                    }
                    // handle all the rest of the switches that may take a second parameter
                         if (arg == "/Fa") ++j;
                    else if (arg == "/Fd") ++j;
                    else if (arg == "/Fe") ++j;
                    else if (arg == "/Fp") ++j;
                    else if (arg == "/Fr") ++j;
                    else if (arg == "/FR") ++j;
                    else if (arg == "/Fi") ++j;
                    else if (arg == "/I")  ++j;
                    else if (arg == "/D")  ++j;
                    else if (arg == "/U")  ++j;
                    else if (arg == "/FI") ++j;
                    else if (arg == "/AI") ++j;
                    else if (arg == "/FU") ++j;
                    else if (arg == "/external:I") ++j;
                    else if (arg == "/Yc") ++j;
                    else if (arg == "/Yu") ++j;
                    else if (arg == "/Tc") ++j;
                    else if (arg == "/Tp") ++j;
                    else if (arg == "/sourceDependencies") ++j;
                    else if (arg == "/analyze:log")        ++j;
                    else if (arg == "/analyze:stacksize")  ++j;

                    continue;
                }
                cppFile = arg;
            }

            std::filesystem::path p(cppFile);
            return fo + p.stem().string() + ".obj";
        }
    };
}
