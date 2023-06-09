#include <iostream>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <locale>

typedef std::unordered_multimap<long long, int> HashMap;

class Parser {

public:

    void ini_parser(const std::string& filename);

    template <typename T> T get_value(std::string title);

private:

    HashMap hash_map;
    std::vector<std::string> variable_map;
    std::ifstream ini_file;
    std::vector<int> search;

    template <typename C> bool variable_search(std::string line, std::string name, C& meaning, bool& variable);

    template <typename D> void working_section(std::string line, std::string name, D& meaning, bool& variable);
};

void Parser::ini_parser(const std::string& filename) {

    ini_file.open(filename);

    // Проверка открытия файла.
    if (ini_file.is_open()) {

        std::string line;
        std::string word;
        std::string error_line = "Неправильная запись в строке ";
        bool section_name = false;
        bool comments = false;
        int num_line = 1;
        int num_section = 0;

        // Построчное считывание файла. 
        for (; std::getline(ini_file, line); num_line++) {

            section_name = false;

            if (line.empty()) {

                continue;
            }

            std::stringstream ini_line(line);

            // Чтение слов из строке.
            for (int num_word = 1; ini_line >> word; num_word++) {

                // Проверка на комментарий.
                if (word.front() == ';') {

                    comments = true;
                    break;
                }

                // Проверка правильности оформления файла.

                else if (word.find(';') != word.npos) {

                    throw std::invalid_argument(error_line += std::to_string(num_line) + ", неразрешенный символ ;");
                }
                // Поиск имени секции.
                else if (word.front() == '[' && word.back() == ']') {

                    //num_section = static_cast<int>(ini_file.tellg()) - static_cast<int>(word.size()) - 2;
                    word.erase(word.begin());
                    word.erase(word.end() - 1);

                    // Создание хеш таблицы с индексами секций
                    hash_map.insert(HashMap::value_type(std::hash<std::string>{}(word), num_line));

                    section_name = true;
                }
                else if (word.find('[') != word.npos) {

                    throw std::invalid_argument(error_line += std::to_string(num_line) + ", неразрешенный символ [.");
                }
                else if (word.find(']') != word.npos) {

                    throw std::invalid_argument(error_line += std::to_string(num_line) + ", неразрешенный символ ].");
                }
                else if (word.front() == '=' && num_word == 1) {

                    throw std::invalid_argument(error_line += std::to_string(num_line) + ", нет имени переменной.");
                }
                else if (word.front() == '=' && num_word > 2) {

                    throw std::invalid_argument(error_line += std::to_string(num_line) + ", ошибка в имени переменной.");
                }
                else if (section_name == true && num_word > 1) {

                    throw std::invalid_argument(error_line += std::to_string(num_line) + ", имя секции, некорректное название.");
                }
            }

            if (line.find('=') == line.npos && section_name == false) {

                if (comments == false) {

                    // В строке переменной не найден символ равенства.
                    throw std::invalid_argument(error_line += std::to_string(num_line) + ", пропущен символ равенства.");
                }
                else {

                    comments = false;
                }
            }
            else if (line.find('=') != line.npos && section_name) {

                // В имени секции найден символ равенства.
                throw std::invalid_argument(error_line += std::to_string(num_line));
            }
        }
    }
    else {

        throw std::invalid_argument("Не получилось открыть файл!");
    }
}

template <typename T> T Parser::get_value(std::string title) {

    std::string section;
    std::string name;
    std::string line;
    std::string word;
    T meaning{};
    int num_index = 0;
    bool variable = false;
    bool section_error = false;

    variable_map.clear();

    // Проверка корректности введенных данных для поиска.
    if (title.empty()) {

        throw std::invalid_argument("Строка поиска пустая, введите   название_секции.имя_переменной   для получения значения.");
    }
    else if (title.find('.') == title.npos) {

        throw std::invalid_argument("Неправильная запись в строке, не найдена '.'.");
    }
    else {

        num_index = static_cast<int>(title.find('.'));

        // Выделение из веденных данных имени секции и название переменной.
        section = name = title;
        section.erase(section.begin() + num_index, section.end());
        name.erase(name.begin(), name.begin() + num_index + 1);

        // Чтение хеш таблицы.
        for (const auto& [hash, num_section] : hash_map) {

            // Поиск имени секции по хеш таблице.
            if (std::hash<std::string>{}(section) == hash) {

                section_error = true;

                // Постановка входящего потока на начало секции.
                ini_file.clear();
                ini_file.seekg(0);

                for (int i = 1; i < num_section; i++) {

                    if (!ini_file.eof()) {

                        std::getline(ini_file, line);
                    }
                }

                if (!ini_file.eof()) {

                    ini_file >> word;

                    word.erase(word.begin());
                    word.erase(word.end() - 1);

                    // Проверка правильности нахождение секции
                    if (section.compare(word) == 0) {

                        working_section(line, name, meaning, variable);
                    }
                }
            }
        }

        if (!section_error) {

            throw std::invalid_argument("В файле отсутствует секция с именем " + section + " .");
        }
        else if (!variable) {

            std::string  error_variable = "В секции " + section + " отсутствует переменная " + name + ". В данной секции находятся следующие переменные: ";

            for (const auto& name_arr : variable_map) {

                error_variable += name_arr + " ";
            }

            throw std::invalid_argument(error_variable);
        }

        return meaning;
    }
}

template <typename D> void Parser::working_section(std::string line, std::string name, D& meaning, bool& variable) {

    while (std::getline(ini_file, line)) {

        if (line.empty()) {

            continue;
        }
        // Определение конца секции. 
        else if (line.find('[') != line.npos || line.find('[') != line.npos) {

            break;
        }
        else {

            if (variable_search<D>(line, name, meaning, variable)) {

                continue;
            }
        }
    }
}

template <typename C> bool Parser::variable_search(std::string line, std::string name, C& meaning, bool& variable) {

    std::string word;
    std::string variable_name;
    int num_index = static_cast<int>(line.find('=') + 1);

    std::stringstream ini_line(line);

    // Поиск переменной.
    if (!ini_line.eof()) {

        ini_line >> word;

        if (word.find(';') != word.npos) {

            return true;
        }

        variable_name = word;

        if (variable_name.find('=') != variable_name.npos) {

            variable_name.erase(variable_name.begin() + variable_name.find('='), variable_name.end());
        }

        variable_map.push_back(variable_name);

        if (variable_name.compare(name) == 0) {

            ini_line.clear();
            ini_line.seekg(num_index);

            // Определение наличия значения переменной.
            if (!ini_line.eof()) {

                ini_line >> word;

                if (word.find(';') != word.npos) {

                    return true;
                }

                variable = true;

                // Чтение значения переменной.
                ini_line.clear();
                ini_line.seekg(num_index);

                ini_line >> meaning;
            }
        }
    }

    return false;
}



int main()
{
    setlocale(LC_ALL, "Russian");

    Parser parser;

    try {

        parser.ini_parser("my.ini");

        auto value = parser.get_value<int>("Section1.var1");
        std::cout << value << std::endl;
    }
    catch (std::invalid_argument& error) {

        std::cerr << error.what() << std::endl;
        return -1;
    }

    return 0;
}