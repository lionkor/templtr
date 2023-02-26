#include <string>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <chrono>
#include <regex>
#include <json/json.h>
#include <fmt/core.h>

void wrong_input() {
    fmt::print("\nPlease specify a command. Available commands are as follows:\n\n\tinit - Init a templtr project\n\tbuild <outdir> - Build project files to specified directory\n\n");
}

std::string string_replace(std::string template_string, std::string key, std::string value) {
    std::regex regex(fmt::format("\\{{{}\\}}", key));
    return std::regex_replace(template_string, regex, value);
}

int init() {
    fmt::print("Initializing project...\n");

    if (!std::filesystem::exists("pages")) {
        std::filesystem::create_directory("pages");
    }

    if (!std::filesystem::exists("content")) {
        std::filesystem::create_directory("content");
    }

    fmt::print("Project initialized!\n");

    return 0;
}

int build(const char *outdir) {
    fmt::print("Started build...\n");

    std::chrono::time_point start_time = std::chrono::high_resolution_clock::now();

    if (std::filesystem::exists(outdir)) {
        std::filesystem::remove_all(outdir);
    }
    
    std::filesystem::create_directory(outdir);

    // Iterate over template files
    for (const auto &page : std::filesystem::directory_iterator("content")) {
        std::string page_name = page.path().stem().string();
        fmt::print("Building {}...\n", page_name);

        std::filesystem::create_directory(fmt::format("{}/{}", outdir, page_name));

        std::string tmpl_path = fmt::format("pages/{}.html", page_name);
        std::string tmpl;

        // Read template file
        if (std::filesystem::exists(tmpl_path)) {
            std::ifstream tmpl_file(tmpl_path, std::ifstream::binary);
            std::stringstream buffer;
            buffer << tmpl_file.rdbuf();
            tmpl = buffer.str();
        }
        else {
            fmt::print("Error: No matching template for {}", page_name);
            return -1;
        }

        std::string content_path = fmt::format("content/{}", page_name);

        // Iterate over dynamic pages
        if (std::filesystem::exists(content_path)) {
            for (const auto &entry : std::filesystem::directory_iterator(content_path)) {
                std::string entry_filename = entry.path().string();
                std::string entry_name = entry.path().stem().string();

                std::ifstream entry_file(entry_filename, std::ifstream::binary);
                Json::Value data;
                entry_file >> data;

                std::string built_page = tmpl;

                // Iterate over keys
                if (data.size() > 0) {
                    for (Json::Value::const_iterator itr = data.begin(); itr != data.end(); itr++) {
                        std::string key = itr.name();
                        
                        if (itr->isString()) {
                            // Handle values of type string
                            std::string value = itr->asString();

                            built_page = string_replace(built_page, key, value);
                        }
                        else if (itr->isArray()) {
                            // Handle values of type array
                            std::string regex = fmt::format("\\[([^\\]]*\\{{{}\\}}[^\\]]*)\\]", key);
                            std::smatch match;

                            // Iterate over array entires
                            while (std::regex_search(built_page, match, std::regex(regex))) {
                                std::string array_item_template = match[1];
                                std::string array_items;

                                for (const auto& item : *itr) {
                                    if (item.isString()) {
                                        array_items += string_replace(array_item_template, itr.name(), item.asString());
                                    }
                                }

                                built_page.replace(built_page.find(match[0].str()), array_item_template.size() + 2, array_items);
                            }                            
                        }
                        else if (itr->isObject()) {
                            for (Json::Value::const_iterator sub_key = itr->begin(); sub_key != itr->end(); sub_key++) {
                                if (sub_key->isString()) {
                                    std::string regex = fmt::format("\\{{{}\\.{}\\}}", key, sub_key.name());

                                    built_page = std::regex_replace(built_page, std::regex(regex), sub_key->asString());
                                }
                            }
                        }
                        else {
                            continue;
                        }
                        // TODO: Add support for object type
                    }
                }

                // Minify
                built_page = std::regex_replace(built_page, std::regex("[\n\r\t]"), "");      

                // Write to file
                std::string out_dir = fmt::format("{}/{}/{}", outdir, page_name, entry_name);
                std::filesystem::create_directory(out_dir);
                std::ofstream out_file(fmt::format("{}/index.html", out_dir));

                out_file << built_page;

                out_file.close();
            }
        }

        fmt::print("Finished building {}\n", page_name);
    }

    // Calculate build duration
    std::chrono::time_point end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration build_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    fmt::print("Build finished in {} milliseconds\n", build_time.count());

    return 0;
}

int main(int argc, char **argv) {
    if (argc == 1) {
        wrong_input();
        return -1;
    }

    if (std::strcmp(argv[1], "init") == 0) {
        init();
    }
    else if (std::strcmp(argv[1], "build") == 0) {
        if (argc == 3) {
            if (build(argv[2]) < 0) {
                fmt::print("Build failed!");
            }
        }
        else {
            wrong_input();
        }
    }
    else {
        wrong_input();
        return -1;
    }

    return 0;
}