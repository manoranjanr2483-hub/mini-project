#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

struct Dependency {
    std::string name, version, license, status;
};

std::string trim(std::string text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    const auto last = text.find_last_not_of(" \t\r\n");
    return first == std::string::npos ? "" : text.substr(first, last - first + 1);
}

std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return text;
}

std::vector<std::string> csvRow(const std::string& line) {
    std::vector<std::string> fields;
    std::string field;
    bool quoted = false;
    for (std::size_t i = 0; i < line.size(); ++i) {
        if (line[i] == '"' && quoted && i + 1 < line.size() && line[i + 1] == '"')
            field += line[++i];
        else if (line[i] == '"')
            quoted = !quoted;
        else if (line[i] == ',' && !quoted) {
            fields.push_back(trim(field));
            field.clear();
        } else
            field += line[i];
    }
    if (quoted) throw std::runtime_error("Unclosed quote in CSV.");
    fields.push_back(trim(field));
    return fields;
}

std::size_t column(const std::vector<std::string>& header, const std::string& name) {
    for (std::size_t i = 0; i < header.size(); ++i)
        if (lower(header[i]) == name) return i;
    throw std::runtime_error("Missing CSV column: " + name);
}

std::vector<Dependency> readDependencies(std::istream& input) {
    std::string line;
    if (!std::getline(input, line)) throw std::runtime_error("Dependencies CSV is empty.");
    auto header = csvRow(line);
    if (!header.empty() && header[0].compare(0, 3, "\xEF\xBB\xBF") == 0)
        header[0].erase(0, 3);

    const auto name = column(header, "name"), version = column(header, "version");
    const auto license = column(header, "license");
    std::vector<Dependency> items;
    while (std::getline(input, line)) {
        if (trim(line).empty()) continue;
        const auto fields = csvRow(line);
        if (fields.size() != header.size()) throw std::runtime_error("Invalid dependency CSV row.");
        if (fields[name].empty()) throw std::runtime_error("Dependency name cannot be empty.");
        items.push_back({fields[name], fields[version], fields[license], ""});
    }
    if (items.empty()) throw std::runtime_error("No dependencies found.");
    return items;
}

std::map<std::string, std::string> readPolicy(std::istream& input) {
    std::string line;
    if (!std::getline(input, line)) throw std::runtime_error("License policy CSV is empty.");
    auto header = csvRow(line);
    if (!header.empty() && header[0].compare(0, 3, "\xEF\xBB\xBF") == 0)
        header[0].erase(0, 3);
    const auto license = column(header, "license"), status = column(header, "status");
    std::map<std::string, std::string> policy;
    while (std::getline(input, line)) {
        if (trim(line).empty()) continue;
        const auto fields = csvRow(line);
        if (fields.size() != header.size()) throw std::runtime_error("Invalid policy CSV row.");
        const auto value = lower(fields[status]);
        if (value != "allowed" && value != "review" && value != "prohibited")
            throw std::runtime_error("Status must be allowed, review, or prohibited.");
        if (!policy.emplace(lower(fields[license]), value).second)
            throw std::runtime_error("Duplicate license: " + fields[license]);
    }
    return policy;
}

std::string csvEscape(const std::string& text) {
    std::string result;
    bool quote = false;
    for (char ch : text) {
        if (ch == '"') { result += "\"\""; quote = true; }
        else { result += ch; quote |= ch == ',' || ch == '\n' || ch == '\r'; }
    }
    return quote ? "\"" + result + "\"" : result;
}

int main(int argc, char* argv[]) {
    if (argc == 2 && std::string(argv[1]) == "--help") {
        std::cout << "Usage: " << argv[0]
                  << " [dependencies.csv] [license_policy.csv] [report.csv]\n";
        return 0;
    }
    if (argc > 4) {
        std::cerr << "Too many arguments. Use --help for usage.\n";
        return 1;
    }

    try {
        std::vector<Dependency> items = {
            {"fmt", "10.2.1", "MIT", ""},
            {"OpenSSL", "3.2.0", "Apache-2.0", ""},
            {"legacy-codec", "1.4.0", "GPL-3.0-only", ""},
            {"image-helper", "2.1.3", "Custom-License", ""},
            {"tiny-parser", "0.9.8", "BSD-3-Clause", ""}
        };
        std::map<std::string, std::string> policy = {
            {"mit", "allowed"}, {"apache-2.0", "allowed"}, {"bsd-2-clause", "allowed"},
            {"bsd-3-clause", "allowed"}, {"isc", "allowed"}, {"mpl-2.0", "review"},
            {"lgpl-2.1-only", "review"}, {"lgpl-3.0-only", "review"},
            {"gpl-2.0-only", "prohibited"}, {"gpl-3.0-only", "prohibited"},
            {"agpl-3.0-only", "prohibited"}
        };

        if (argc > 1) {
            std::ifstream file(argv[1]);
            if (!file) throw std::runtime_error("Cannot open dependencies file.");
            items = readDependencies(file);
        }
        if (argc > 2) {
            std::ifstream file(argv[2]);
            if (!file) throw std::runtime_error("Cannot open policy file.");
            policy = readPolicy(file);
        }

        for (auto& item : items) {
            const auto found = policy.find(lower(item.license));
            item.status = found == policy.end() ? "review" : found->second;
        }
        std::sort(items.begin(), items.end(), [](const auto& a, const auto& b) {
            return lower(a.name) < lower(b.name);
        });

        int allowed = 0, review = 0, prohibited = 0;
        std::cout << "Dependency License Audit\n------------------------\n";
        for (const auto& item : items) {
            auto label = item.status;
            std::transform(label.begin(), label.end(), label.begin(), [](unsigned char ch) {
                return static_cast<char>(std::toupper(ch));
            });
            std::cout << '[' << label << "] " << item.name << ' ' << item.version << " - "
                      << (item.license.empty() ? "(missing)" : item.license) << '\n';
            if (item.status == "allowed") ++allowed;
            else if (item.status == "prohibited") ++prohibited;
            else ++review;
        }
        std::cout << "\n" << items.size() << " dependencies: " << allowed << " allowed, "
                  << review << " for review, " << prohibited << " prohibited.\n";

        if (argc > 3) {
            std::ofstream report(argv[3]);
            if (!report) throw std::runtime_error("Cannot create report file.");
            report << "name,version,license,status\n";
            for (const auto& item : items)
                report << csvEscape(item.name) << ',' << csvEscape(item.version) << ','
                       << csvEscape(item.license) << ',' << item.status << '\n';
            if (!report) throw std::runtime_error("Failed to write report.");
        }
        return review || prohibited ? 2 : 0;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
