// truetype.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <TrueTypeReader.h>
#include <windows.h>

#ifdef UNICODE
#define printf wprintf

using string = std::wstring;
#else
using string = std::string;
#endif

// 需要注意一些小地方:类型、参数、引用。。。

std::string getDefaultGuiFontName() {
	HFONT defaultFont1 = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	LOGFONTA lf;
	GetObjectA(defaultFont1, sizeof(lf), &lf);
	return std::string(lf.lfFaceName);
}
std::string getDefaultSystemFontName() {
	NONCLIENTMETRICSA metrics;
	metrics.cbSize = sizeof(metrics);
	SystemParametersInfoA(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0);
	return std::string(metrics.lfMessageFont.lfFaceName);
}

int main()
{
#ifdef UNICODE
	setlocale(LC_ALL, "chs");
#endif
	TrueType::FontReader fontReader;
	TrueType::NameTable nameTable;
	TrueType::FontsRegTableReader fontsRegTableReader;
	fontReader.loadFontData(TEXT("宋体"),"name");
	nameTable.read(fontReader);
	nameTable.Printf();
	fontsRegTableReader.loadfontsinfo();
	string fontpath=fontsRegTableReader.getfilepath(nameTable.getName(3, 1033, 1));
	printf(TEXT("%s:%s\n"), nameTable.getName(3, 2052, 1).c_str(), fontpath.c_str());
	printf(TEXT("%s\n"),nameTable.getName(3, 1033, 1).c_str());
	printf(TEXT("%s\n"),nameTable.getName(3, 2052, 1).c_str());
	//std::cout << nameTable.getName(3, 1033, 1) <<"\n";
	//std::cout << nameTable.getName(3, 2052, 1) << "\n";
    std::cout << "Hello World!\n";
}

#ifdef UNICODE
#undef printf 
#endif


// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
