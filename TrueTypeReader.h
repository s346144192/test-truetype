#pragma once
// 文档:https://learn.microsoft.com/en-us/typography/opentype/spec/otff

#include <stdint.h>
#include <locale>
#include <string>
#include <vector>
#include <unordered_map>
#include <windows.h>

// 格式
namespace TrueType {

#ifdef UNICODE
	#define printf wprintf

	using string = std::wstring;
#else
	using string = std::string;
#endif
	// https://github.com/tommccallum/opentype_font_explorer
	class FontReader {
		//friend NameTable;
	public:
		std::vector<uint8_t> data;
		int pos;
		FontReader() :pos(0) {}
		FontReader(std::vector<uint8_t> & _data) :pos(0) {
			data = _data;
		}
		uint8_t getbyte() {
			if (pos < data.size()) {
				pos++;
				return (uint8_t)data[pos - 1];
			}
			return 0;
		}
		std::vector<uint8_t> getbytes(size_t length) {
			size_t start = pos;
			size_t end = pos+ length-1;
			if (end >= data.size()) {
				end = data.size() - 1;
			}
			std::vector<uint8_t> bytes;
			if (start <= end) {
				length = end - start + 1;
				bytes.resize(length);
				memcpy(&bytes[0], &data[start], length); // string.length 不建议使用
			}
			return bytes;
		}

		uint16_t readUInt16() {
			uint16_t i = (getbyte() << 8);
			i = i + getbyte();
			return i;
		}
		int16_t readInt16() {
			int16_t i = (getbyte() << 8);
			i = i + getbyte();
			return i;
		}
		uint32_t readUInt32() {
			uint32_t i = (getbyte() << 24);
			i = i + (getbyte() << 16);
			i = i + (getbyte() << 8);
			i = i + getbyte();
			return i;
		}
		int32_t readInt32() {
			int32_t i = (getbyte() << 24);
			i = i + (getbyte() << 16);
			i = i + (getbyte() << 8);
			i = i + getbyte();
			return i;
		}
		string readStringUTF8(int offset, int length) {
			offset += pos;
			std::string str;
			for (int ii = offset; ii < offset + length; ii++) {
				str.push_back(data[ii]);
			}
		#ifdef UNICODE
			std::wstring wstr;
			wstr.resize(str.size()+1);
			MultiByteToWideChar(CP_UTF8, 0, str.c_str(), str.size(), (wchar_t*)wstr.c_str(), wstr.size());
			return wstr;
		#else
			return str;
		#endif
		}
		string readStringUTF16BF(int offset, int length) {
			offset += pos;
			std::wstring wstr;
			wchar_t ch = 0;
			for (int ii = offset; ii < offset + length && ii+1<data.size(); ii += 2) {
				ch = (((uint8_t)data[ii]) << 8)+((uint8_t)data[ii + 1]);
				wstr.push_back(ch);
			}
		#ifdef UNICODE
			return wstr;
		#else
			std::string str;
			str.resize((wstr.length() + 1) * 2);
			WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), wstr.size(), (char*)str.c_str(), str.size(), '\0', FALSE);
			return str;
		#endif
		}
		bool loadFontData(HFONT font, const char* tagid) {
			data.clear();
			pos = 0;
			if (font == nullptr) {
				return false;
			}
			DWORD tag = 0;
			if (tagid != nullptr) {
				tag = *(DWORD*)tagid;
			}
			HDC hdc = GetDC(NULL);
			SelectObject(hdc, font);
			DWORD datasize = GetFontData(hdc, tag, 0, 0, 0);
			if (datasize == 0 || datasize == GDI_ERROR) {
				ReleaseDC(NULL, hdc);
				return false;
			}
			data.resize(datasize);
			GetFontData(hdc, tag, 0, &data[0], datasize);
			ReleaseDC(NULL, hdc);
			return true;
		}
		bool loadFontData(string fontname, const char* tagid) {
			HFONT font = CreateFont(
				-15						/*高度*/,
				-7.5					/*宽度*/,
				0						/*不用管*/, 
				0						/*不用管*/, 
				400						/*一般这个值设为400*/,
				FALSE					/*不带斜体*/,
				FALSE					/*不带下划线*/,
				FALSE					/*不带删除线*/,
				DEFAULT_CHARSET,		//这里我们使用默认字符集，还有其他以 _CHARSET 结尾的常量可用
				OUT_CHARACTER_PRECIS,	//这行参数不用管
				CLIP_CHARACTER_PRECIS,	//这行参数不用管
				DEFAULT_QUALITY,		//默认输出质量
				FF_DONTCARE,			//不指定字体族*/
				fontname.c_str()		//字体名
			);
			bool success =loadFontData(font, tagid);
			DeleteObject(font);
			return success;
		}

	};

	typedef struct NameRecord
	{
		uint16_t platformID;
		uint16_t encodingID;
		uint16_t languageID;
		uint16_t nameID;
		uint16_t length;
		uint16_t offset;
		string text;
	};
	typedef struct LangTagRecord
	{
		uint16_t length;
		uint16_t langTagOffset;
		string text;
	};

	typedef struct NameTableHeader
	{
		uint16_t formatId;
		uint16_t nRecord;
		uint16_t Offset;
	};

	struct NameTable {
		NameTableHeader header;
		std::vector<NameRecord> records;
		uint16_t langTag = 0;
		std::vector<LangTagRecord> langTagRecords;
		enum PlatformIDs {
			PID_UNICODE=0,
			PID_MACOS,
			PID_UNKNOWN1,
			PID_WINDOWS,
		};
		enum EncodingIDs {
			ENC_SYMBOL = 0,
			ENC_UNICODE_BMP,
			ENC_SHIFTJIS,
			ENC_PRC,
			ENC_BIG5,
			ENC_WANSUNG,
			ENC_JOHAB,
			ENC_RESERVED1,
			ENC_RESERVED2,
			ENC_RESERVED3,
			ENC_UNICODE_FULL,
		};
		bool read(FontReader& fontReader) {
			header.formatId = fontReader.readUInt16();
			header.nRecord = fontReader.readUInt16();
			header.Offset = fontReader.readUInt16();
			for (int i = 0; i < header.nRecord; i++) {
				NameRecord record;
				record.platformID = fontReader.readUInt16();
				record.encodingID = fontReader.readUInt16();
				record.languageID = fontReader.readUInt16();
				record.nameID = fontReader.readUInt16();
				record.length = fontReader.readUInt16();
				record.offset = fontReader.readUInt16();
				records.push_back(std::move(record));
			}
			if (header.formatId == 1) {
				langTag = fontReader.readUInt16();
				for (int i = 0; i < langTag; i++) {
					LangTagRecord record;
					record.length = fontReader.readUInt16();
					record.langTagOffset = fontReader.readUInt16();
					langTagRecords.push_back(std::move(record));
				}
			}
			for (auto& record:records) {
				switch (record.platformID)
				{
				case PID_MACOS:
					switch (record.encodingID) {
					case ENC_SYMBOL:
						record.text = fontReader.readStringUTF8(record.offset, record.length);
						// printf("macos-symbol:%s\n", record.text.c_str());
						break;
					default:
						break;
					}
					break;
				case PID_WINDOWS:
					switch (record.encodingID) {
					case ENC_SYMBOL:
						record.text = fontReader.readStringUTF16BF(record.offset, record.length);
						// printf("win-symbol-languageID-%d:nameID:%d,Offset:%d,length:%d -> %s\n", record.languageID, record.nameID,record.offset, record.length, record.text.c_str());
						break;
					case ENC_UNICODE_BMP:
						record.text = fontReader.readStringUTF16BF(record.offset, record.length);
						// printf("win-unicode-bmp-languageID-%d:nameID:%d,Offset:%d,length:%d -> %s\n", record.languageID, record.nameID,  record.offset, record.length, record.text.c_str());
						break;
					default:
						break;
					}
					break;
				default:
					break;
				}
			}
			if (header.formatId == 1) {
				for (auto& ragRecord : langTagRecords) {
					ragRecord.text= fontReader.readStringUTF16BF(ragRecord.langTagOffset, ragRecord.length);
					// printf("rag_record:%s\n", rag_record.text.c_str());
				}
			}
			return true;
		}

		void Printf() {
			#ifdef UNICODE
				setlocale(LC_ALL, "chs");
			#endif
			for (auto& record : records) {
				switch (record.platformID)
				{
				case PID_MACOS:
					switch (record.encodingID) {
					case ENC_SYMBOL:
						printf(TEXT("macos-symbol:%s\n"), record.text.c_str());
						break;
					default:
						break;
					}
					break;
				case PID_WINDOWS:
					switch (record.encodingID) {
					case ENC_SYMBOL:
						printf(TEXT("win-symbol-languageID-%d:nameID:%d,Offset:%d,length:%d -> %s\n"), record.languageID, record.nameID, record.offset, record.length, record.text.c_str());
						break;
					case ENC_UNICODE_BMP:
						printf(TEXT("win-unicode-bmp-languageID-%d:nameID:%d,Offset:%d,length:%d -> %s\n"), record.languageID, record.nameID, record.offset, record.length, record.text.c_str());
						break;
					default:
						break;
					}
					break;
				default:
					break;
				}
			}
			if (header.formatId == 1) {
				for (auto& ragRecord : langTagRecords) {
					printf(TEXT("rag_record:%s\n"), ragRecord.text.c_str());
				}
			}
		}
		string getName(uint16_t platformID, uint16_t languageID, uint16_t nameId) {
			for (auto& record : records) {
				if (record.platformID == platformID && record.languageID == languageID && record.nameID== nameId) {
					return record.text;
				}
			}
			return string();
		}
	
	};

	class FontsRegTableReader {
	public:
		std::unordered_map<string, string> table;
		static std::vector<string> splitNameData(string source) {
			std::vector<string> names;
			size_t sp = source.find_last_of(TEXT('('));
			if (sp != std::string::npos)
				source = source.substr(0, sp);
			string buf;
			for (auto c:source) {
				if (c == TEXT('&') && buf.size() > 0 && buf.back() == TEXT(' ')) {
					buf.pop_back();
					names.push_back(buf);
					buf.clear();
				}
				else if(c != TEXT(' ') || buf.size() > 0){
					buf.push_back(c);
				}
			}
			if (buf.size() > 0) {
				names.push_back(buf);
				buf.clear();
			}
			return names;
		}

		bool putfontinfo(string source,string filename) {
			std::vector<string> names = splitNameData(source);
			for (auto it : names) {
				table[it] = filename;
			}
			return (names.size() > 0);
		}

		bool loadfontsinfo() {
			// https://qa.1r1g.com/sf/ask/797129511/
			static const string fontRegistryPath = TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Fonts");
			HKEY hKey=nullptr;
			LONG result=0;
			result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, fontRegistryPath.c_str(), 0, KEY_READ, &hKey);
			if (result != ERROR_SUCCESS) {
				return false;
			}
			DWORD maxValueNameSize, maxValueDataSize;
			result = RegQueryInfoKey(hKey, 0, 0, 0, 0, 0, 0, 0, &maxValueNameSize, &maxValueDataSize, 0, 0);
			if (result != ERROR_SUCCESS) {
				return false;
			}

			DWORD valueIndex = 0;
		#ifdef UNICODE
			LPWSTR valueName = new WCHAR[maxValueNameSize];
		#else
			LPSTR valueName = new CHAR[maxValueNameSize];
		#endif
			LPBYTE valueData = new BYTE[maxValueDataSize];
			DWORD valueNameSize, valueDataSize, valueType;
			string wsFontFile;
			do {

				wsFontFile.clear();
				valueDataSize = maxValueDataSize;
				valueNameSize = maxValueNameSize;

				result = RegEnumValue(hKey, valueIndex, valueName, &valueNameSize, 0, &valueType, valueData, &valueDataSize);

				valueIndex++;

				if (result != ERROR_SUCCESS || valueType != REG_SZ) {
					continue;
				}

				string sValueName(valueName, valueNameSize);
			#ifdef UNICODE
				string sValueData((const wchar_t*)valueData, valueDataSize);
			#else
				string sValueData((const char*)valueData, valueDataSize);
			#endif
				putfontinfo(sValueName, sValueData);
			} while (result != ERROR_NO_MORE_ITEMS);
			delete[] valueName;
			delete[] valueData;

			RegCloseKey(hKey);
			
		}

		static string getsysfontsdir() {
		#ifdef UNICODE
			wchar_t sysDir[MAX_PATH];
		#else
			char sysDir[MAX_PATH];
		#endif
			GetWindowsDirectory(sysDir, MAX_PATH);
			string dir(sysDir);
			dir.append(TEXT("\\fonts\\"));
			return dir;
		}
		string convertfilename(string& str) {
			if (size_t sp = str.find_last_of(TEXT('\\')) != std::string::npos)
				return str.substr(sp);
			if (size_t sp = str.find_last_of(TEXT('/')) != std::string::npos)
				return str.substr(sp);
			return str;
		}
		string convertfilepath(string& str) {
			static string sysfontdir = getsysfontsdir();
			if (str.find_last_of(TEXT('\\')) != std::string::npos || str.find_last_of(TEXT('/')) != std::string::npos)
				return str;
			return sysfontdir+str;
		}
		string getfilename(string fontname) {
			auto it = table.find(fontname);
			if (it!=table.end()) {
				return convertfilename(it->second);
			}
			return string();
		}
		string getfilepath(string fontname) {
			auto it = table.find(fontname);
			if (it != table.end()) {
				return convertfilepath(it->second);
			}
			return string();
		}
	};



#ifdef UNICODE
	#undef printf 
#endif

}
