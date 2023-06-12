#include <iostream>
#include <fstream>
#include <string>
#include <set>
#include <algorithm>
#include <exception>

class BadIni : public std::runtime_error
{
public:
	BadIni(std::string err) : std::runtime_error(err) {};
};

class ParserIniFile
{
public:
	ParserIniFile() = default;
	
	explicit ParserIniFile(const std::string &filename)
	{
		try 
		{
			_filename = filename;
			_file.exceptions(std::ios::failbit | std::ios::badbit);
			_file.open(filename);			
		}
		catch (const std::ios::failure &ex)
		{
			std::string message = "Ошибка открытия файла: " + filename + "\nКод ошибки: " + std::to_string(ex.code().value());
			throw BadIni(message);
		}
		catch (const std::exception &ex)
		{
			std::string message = std::to_string(*ex.what());
			throw BadIni(message);
		}
	};
	
	template<typename T>
	T get_value(const std::string &sec_var);

	~ParserIniFile()
	{
		_file.close();
	};
private:
	std::ifstream _file;
	std::string _filename;
	const char SECTION_OPEN = '[';
	const char SECTION_CLOSE = ']';
	const char COMMENT = ';';
	const char ASSIGN = '=';
	const char NEW_LINE = '\n';
	const char FLOAT_DIV = '.';
	const char SPACES[3] = " \t";

	void cut_spaces(std::string &line)
	{
		size_t pos_first_symbol = line.find_first_not_of(SPACES, 0);
		if (pos_first_symbol != std::string::npos)
		{
			line.erase(0, pos_first_symbol);
		}
		size_t pos_last_symbol = line.find_last_not_of(SPACES, line.length() - 1);
		if (pos_last_symbol != std::string::npos)
		{
			line.erase(pos_last_symbol + 1, line.length());
		}
		if (line.length() > 0 && pos_first_symbol == std::string::npos && pos_last_symbol == std::string::npos)//в строке только пробелы
		{
			line.clear();
		}
	}
};

template<typename T>
T ParserIniFile::get_value(const std::string &sec_var)
{
	T result_value;
	std::string sec_name, var_name, line, current_sec, current_var;
	int line_count = 0;
	bool not_find_sec = true;
	bool not_find_var = true;
	std::set<std::string> variables;
	//контроль аргумента функции get_value() и разделение наsection и value
	int pos_dot = sec_var.find('.');
	if (pos_dot == std::string::npos)
	{
		std::string message = "Неверный формат аргумента \"" + sec_var +
			"\" функции get_value(), наименования секции и переменной должны быть разделены точкой.";
		throw std::invalid_argument(message);
	}
	else
	{
		sec_name = sec_var.substr(0, pos_dot);
		var_name = sec_var.substr(pos_dot + 1, sec_var.length() - pos_dot);
		//перевод в нижний регистр для возможности регистронезависимого указания наименования секции
		std::for_each(sec_name.begin(), sec_name.end(), [](char &ch) { ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch))); });
		if (sec_name.length() == 0 || var_name.length() == 0)
		{
			std::string message = "Неверный формат аргумента \"" + sec_var +
				"\" функции get_value(), не указано наименование секции или переменной. \nКорректный формат: section.variable.";
			throw std::invalid_argument(message);
		}
		for (int i = 0; i < sec_name.length(); ++i)
		{
			char a = sec_name[i];
			if (std::ispunct(a) || std::iscntrl(a) || std::isspace(a))
			{
				std::string message = "В наименовании секции \"" + sec_name + "\" аргумента \"" + sec_var +
					"\" функции get_value() содержатся недопустимые символы.";
				throw std::invalid_argument(message);
			}
		}
		for (int i = 0; i < var_name.length(); ++i)
		{
			char a = var_name[i];
			if (std::ispunct(a) || std::iscntrl(a) || std::isspace(a))
			{
				std::string message = "В наименовании переменной \"" + var_name + "\" аргумента \"" + sec_var +
					"\" функции get_value() содержатся недопустимые символы.";
				throw std::invalid_argument(message);
			}
		}
	}
	//формирование строки без пробелов, комментариев и командных символов
	while (!_file.eof())
	{
		line.clear();
		
		std::getline(_file, line);
		line_count++;
		//удаление комментариев в строке
		size_t pos_comment = line.find(COMMENT);
		if(pos_comment != std::string::npos)
		{
			line.erase(pos_comment, line.length());
		}
		//удаление незначащих пробелов в начале и конце наименования секции
		cut_spaces(line);
		if (line == "")
		{
			continue;
		}
		//поиск символов открытия и закрытия секции []
		size_t pos_open = line.find(SECTION_OPEN);
		size_t pos_close = line.find(SECTION_CLOSE);		
		
		if (pos_open != std::string::npos || pos_close != std::string::npos)//в строке есть хотя бы одна скобка [] - вероятно это заголовок секции
		{
			if (pos_open != std::string::npos && pos_close != std::string::npos && pos_open < pos_close)//найдены обе скобки в строке и они стоят в правильном порядке
			{
				current_sec = line.substr(pos_open + 1, pos_close - pos_open - 1);
				//удаление незначащих пробелов в начале и конце наименования секции, т.е. будет допустимо [  section1  ]
				cut_spaces(current_sec);
				//перевод в нижний регистр для возможности регистронезависимого указания наименования секции
				std::for_each(current_sec.begin(), current_sec.end(), [](char &ch) { ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch))); });
				if (current_sec == sec_name)
				{
					not_find_sec = false;//секция найдена			
				}
				else
				{
					continue;
				}
			}
			else if (pos_open != std::string::npos && pos_close == std::string::npos)//есть только скобка ]
			{
				std::string message = "Синтаксическая ошибка в файле \"" + _filename + "\" в строке " + std::to_string(line_count) + 
					". Нет закрывающей скобки ].";
				throw BadIni(message);
			}
			else if (pos_open == std::string::npos && pos_close != std::string::npos)//есть только скобка [
			{
				std::string message = "Синтаксическая ошибка в файле \"" + _filename + "\" в строке " + std::to_string(line_count) + 
					". Нет открывающей скобки [.";
				throw BadIni(message);
			}
			else if (pos_open > pos_close)//скобки в неправильном порядке ][
			{
				std::string message = "Синтаксическая ошибка в файле \"" + _filename + "\" в строке " + std::to_string(line_count) + 
					". Скобки ][ в неправильном порядке.";
				throw BadIni(message);
			}
		}
		else if (current_sec == sec_name)//находимся внутри секции
		{
			size_t pos_assign = line.find(ASSIGN);
			current_var = line.substr(0, pos_assign);
			//удаление незначащих пробелов в начале и конце наименования переменной
			cut_spaces(current_var);
			variables.insert(current_var);
			if (current_var == var_name)
			{
				if ((current_var.length() == line.length() - 1) || (pos_assign == std::string::npos))//после = ничего не написано или знака = нет в строке с нужной переменной
				{
					std::string message = "Синтаксическая ошибка в файле \"" + _filename + "\" в строке " + std::to_string(line_count) + 
						". Не задано значение переменной \"" + current_var + "\"";
					throw BadIni(message);
				}
				std::string value_string = line.substr(pos_assign + 1, line.length() - pos_assign);
				//удаление незначащих пробелов в начале и конце значения переменной, предполагается, что пробелы по краям не имеют значения для тектовых переменных
				cut_spaces(value_string);
				not_find_var = false;
				size_t pos_err;
				if constexpr (std::is_floating_point<T>::value)
				{
					std::string prev_loc = std::setlocale(LC_ALL, nullptr);//запись текущей локали
					try
					{
						//атоматический выбор локали для std::stold в зависимости от поля FLOAT_DIV
						if (FLOAT_DIV == '.')
						{
							setlocale(LC_NUMERIC, "C");
						}
						else if (FLOAT_DIV == ',')
						{
							setlocale(LC_NUMERIC, "RU");							
						}
						if constexpr (std::is_same<T, float>::value)
						{
							result_value = std::stof(value_string, &pos_err);
						}
						else if constexpr (std::is_same<T, double>::value)
						{
							result_value = std::stod(value_string, &pos_err);
						}
						else if constexpr (std::is_same<T, long double>::value)
						{
							result_value = std::stold(value_string, &pos_err);
						}						
						std::setlocale(LC_ALL, prev_loc.c_str());//возврат локали при успешном преобразовании
						if (pos_err != value_string.length())//преобразовалась не вся строка после знака =
						{
							std::string message;
							if (FLOAT_DIV == '.' && value_string.find(',') != std::string::npos)//если разделителем считается точка, а найдена запятая
							{
								message = "Синтаксическая ошибка в файле \"" + _filename + "\" в строке " + std::to_string(line_count) +
									". Некорректное значение переменной \"" + current_var + " = " + value_string + "\" - в качестве разделителя должна быть указана точка \".\".";
							}
							else if (FLOAT_DIV == ',' && value_string.find('.') != std::string::npos)//если разделителем считается запятая, а найдена точка
							{
								message = "Синтаксическая ошибка в файле \"" + _filename + "\" в строке " + std::to_string(line_count) +
									". Некорректное значение переменной \"" + current_var + " = " + value_string + "\" - в качестве разделителя должна быть указана запятая \",\".";
							}
							else
							{
								message = "Синтаксическая ошибка в файле \"" + _filename + "\" в строке " + std::to_string(line_count) + 
									". Некорректное значение переменной \"" + current_var + " = " + value_string + "\" - не соответсвует типу c плавающей точкой.";
							}							
							throw BadIni(message);
						}
					}
					catch (std::invalid_argument)
					{
						std::setlocale(LC_ALL, prev_loc.c_str());//возврат локали при неуспешном преобразовании
						std::string message = "Ошибка конвертирования строки \"" + value_string + "\" в число c плавающей точкой. std::invalid_argument";
						throw (std::invalid_argument(message));
					}
					catch (std::out_of_range)
					{
						std::setlocale(LC_ALL, prev_loc.c_str());//возврат локали при неуспешном преобразовании
						std::string message = "Ошибка конвертирования строки \"" + value_string + "\" в число c плавающей точкой. std::out_of_range";
						throw (std::out_of_range(message));
					}					
				}
				else if constexpr (std::is_same<T, int>::value || std::is_same<T, long>::value || std::is_same<T, long long>::value ||
								std::is_same<T, unsigned long>::value || std::is_same<T, unsigned long long>::value)
				{
					try
					{
						if constexpr (std::is_same<T, int>::value)
						{
							result_value = std::stoi(value_string, &pos_err);
						}							
						else if constexpr (std::is_same<T, long>::value)
						{
							result_value = std::stol(value_string, &pos_err);
						}							
						else if constexpr (std::is_same<T, long long>::value)
						{
							result_value = std::stoll(value_string, &pos_err);
						}						
						else if constexpr (std::is_same<T, unsigned long>::value)
						{
							result_value = std::stoul(value_string, &pos_err);
						}							
						else if constexpr (std::is_same<T, unsigned long long>::value)
						{
							result_value = std::stoull(value_string, &pos_err);
						}
						if (pos_err != value_string.length())//преобразовалась не вся строка после знака =
						{
							std::string message = "Синтаксическая ошибка в файле \"" + _filename + "\" в строке " + std::to_string(line_count) + 
								". Некорректное значение переменной \"" + current_var + " = " + value_string + "\" - не соответсвует целому типу.";
							throw BadIni(message);
						}
					}
					catch (std::invalid_argument)
					{
						std::string message = "Ошибка конвертирования строки \"" + value_string + "\" в целое число. std::invalid_argument";
						throw (std::invalid_argument(message));
					}
					catch (std::out_of_range)
					{
						std::string message = "Ошибка конвертирования строки \"" + value_string + "\" в целое число. std::out_of_range";
						throw (std::out_of_range(message));
					}
				}
				else
				{
					result_value = line.substr(pos_assign + 1, line.length() - pos_assign);
				}
			}
		}
	}	
	if (not_find_sec)
	{
		std::string message = "Искомая секция \"" + sec_name + "\" не найдена.";
		throw BadIni(message);
	}
	else if (not_find_var)
	{
		std::string message = "Искомая переменная \"" + var_name + "\" не найдена в секции \"" + sec_name +"\".\nНайдены следующие переменные:\n";
		for (const auto &var : variables)
		{
			message = message + var + '\n';
		}
		throw (std::out_of_range(message));
	}
	return result_value;
};

void write_ini(int i, std::string filename);//функция создания ini_file для удобства тестирования, определение функции после main

template<typename T>//вспомогательная функция для тестирования
void help_test_function(std::string filename, std::string get_argument)
{
	try
	{
		ParserIniFile parser(filename);
		T value = parser.get_value<T>(get_argument);
		std::cout << std::fixed << "\x1B[32m" << "Возвращена переменная типа " << typeid(value).name() 
			<< " " << get_argument << "(" << filename << ") = " << value << "\x1B[0m" << std::endl;
	}
	catch (const std::exception &ex)
	{
		std::cout << "\x1B[31m" << ex.what() << "\x1B[0m" << std::endl;
	}
	std::cout << std::endl;
}

int main()
{
	std::setlocale(LC_ALL, "RU");
	//HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	//SetConsoleTextAttribute(hConsole, 12);
	
	//создание file.ini
	write_ini(1, "file1.ini");
	write_ini(2, "file2.ini");
	write_ini(3, "file3.ini");
	write_ini(4, "file4.ini");
	write_ini(5, "file5.ini");
	
	//ошибка открытия файла, указано неверное имя фалйа	
	std::cout << "\x1B[36m-----Ошибка в имени файла-----\x1B[0m\n";
	help_test_function<int>("file7.ini", "section1.var1");
	
	//ошибка в имена аргумента переданного в get_value
	std::cout << "\x1B[36m-----Ошибки в имени аргумента get_value()-----\x1B[0m\n";
	//1 - нет точки
	help_test_function<int>("file1.ini", "section1var1");
	//2 - в имени секции спец-символ
	help_test_function<int>("file1.ini", "secti#on1.var1");
	//3 - в имени переменной спец-символ
	help_test_function<int>("file1.ini", "section1.va r1");	

	//синтаксические ошибки в имени секции
	std::cout << "\x1B[36m-----Ошибки в имени секции-----\x1B[0m\n";
	//1 - нет закрывающей скобки ]
	help_test_function<double>("file2.ini", "section1.var1");
	//2 - нет открывающей скобки [
	help_test_function<double>("file3.ini", "section1.var1");
	//3 - скобки в неверном порядке ][
	help_test_function<double>("file4.ini", "section1.var1");

	//В строке с искомой переменной отсутствует знак = или ничего не указано после него
	std::cout << "\x1B[36m-----Переменной не присвоено значение-----\x1B[0m\n";
	//1 - после равно ничего не написано
	help_test_function<double>("file5.ini", "section4.Mode");
	//2 - нет знака равно после Vid
	help_test_function<double>("file5.ini", "section4.Vid");

	//Работа с целыми числами
	std::cout << "\x1B[36m-----ЦЕЛЫЕ ЧИСЛА-----\x1B[0m\n";
	//1 - в файле в переменной double значение, а запрос на целое
	help_test_function<int>("file1.ini", "section1.var1");
	//2 - в файле в переменной текстовое значение, а запрос на целое
	help_test_function<int>("file1.ini", "section1.var2");
	//3 - искомая секция не найдена в файле
	help_test_function<int>("file1.ini", "section15.var2");
	//4 - искомая переменная не найдена в файле
	help_test_function<int>("file1.ini", "section1.var25");
	//5 - выход за границу int
	help_test_function<int>("file5.ini", "section2.var1");
	//6 - то же самое, что предыдущее, но тип принят long long, чтобы число попало в границы типа
	help_test_function<long long>("file5.ini", "section2.var1");
	//7 - нормальная работа с int
	help_test_function<int>("file1.ini", "section2.var1");

	//Работа с числами c плавающей точкой
	std::cout << "\x1B[36m-----ЧИСЛА С ПЛАВАЮЩЕЙ ТОЧКОЙ-----\x1B[0m\n";
	//1 - в файле в переменной текстовое значение, а запрос на double
	help_test_function<double>("file1.ini", "section1.var2");
	//2 - выход за границу float
	help_test_function<float>("file5.ini", "section2.var2");
	//3 - то же самое, что предыдущее, но тип принят double, чтобы число попало в границы типа
	help_test_function<double>("file5.ini", "section2.var2");
	//4 - в double запятая вместо точки
	help_test_function<double>("file5.ini", "section1.var3");
	//5 - нормальная работа с double
	help_test_function<double>("file1.ini", "section1.var1"); //должен выдать значение в последней секции section1, перезаписанное

	//Работа со строковыми переменными
	std::cout << "\x1B[36m-----СТРОКОВЫЕ ПЕРЕМЕННЫЕ-----\x1B[0m\n";
	help_test_function<std::string>("file1.ini", "section2.var2");
	help_test_function<std::string>("file1.ini", "section1.var2");
	help_test_function<std::string>("file1.ini", "section1.var3");
	
	return 0;
}

void write_ini(int i, std::string filename)
{
	std::ofstream file(filename);
	std::string str;
	switch (i)
	{
//---------------------------------------------------------------------------------------------
	case 1://пример из задания
		str = R"inifile([Section1]
; комментарий о разделе
var1=5.0 ; иногда допускается комментарий к отдельному параметру
var2=какая-то строка
  
[Section2]
var1=1
var2=значение_2

; Иногда значения отсутствуют, это означает, что в Section3 нет переменных 
[Section3]
[Section4]
Mode=
Vid=

; Секции могут повторяться
[Section1]
var3=значение
var1=1.0 ; переприсваиваем значение)inifile";		
		break;
//---------------------------------------------------------------------------------------------
	case 2://нет скобки ]
		str = R"inifile([Section1]
; комментарий о разделе
var1=5.0 ; иногда допускается комментарий к отдельному параметру
var2=какая-то строка
  
[Section2
var1=1
var2=значение_2)inifile";
		break;
//---------------------------------------------------------------------------------------------
	case 3://нет скобки [
		str = R"inifile([Section1]
; комментарий о разделе
var1=5.0 ; иногда допускается комментарий к отдельному параметру
var2=какая-то строка
  
Section2]
var1=1
var2=значение_2)inifile";
		break;
//---------------------------------------------------------------------------------------------
	case 4://скобки в неверном порядке ][
		str = R"inifile([Section1]
; комментарий о разделе
var1=5.0 ; иногда допускается комментарий к отдельному параметру
var2=какая-то строка
  
]Section2[
var1=1
var2=значение_2)inifile";
		break;
//---------------------------------------------------------------------------------------------
	case 5://Mode=<нет переменной>; Vid<нет знака равно>, section2.var1 - за границами int, section2.var2 - за границами float, section1.var3 -запятая вместо точки
		str = R"inifile([Section1]
; комментарий о разделе
var1=5.0 ; иногда допускается комментарий к отдельному параметру
var2=какая-то строка
  
[Section2]
var1=      2147483648
var2=3.4028235e+39

; Иногда значения отсутствуют, это означает, что в Section3 нет переменных 
[Section3]
[Section4]
Mode=
Vid

; Секции могут повторяться
[Section1]
var3=3,56
var1=1.0 ; переприсваиваем значение)inifile";
		break;
//---------------------------------------------------------------------------------------------
	}
	file << str;
	file.close();
}