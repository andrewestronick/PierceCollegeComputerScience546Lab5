// Estronick, Andrew CS546 Section 3122  June 2, 2017
// Lab Assignment 5 – Search bitmap for copyright colors

#define PICTURE_FILE "c:\\temp\\CS546Lab5TestImage.bmp"
#define COLOR_FILE "c:\\temp\\Colors64.txt"
const int HASH_SIZE = ( 1024 * 16 );

#include<iostream>
#include<vector>
#include<fstream>
#include<sstream>
#include "windows.h"

struct entry
{
	unsigned key;
	std::string name;
};

ULONGLONG getTime64(LPFILETIME a);

class HashEntry
{
	public:

	HashEntry(unsigned key, std::string value1) { this->key = key; this->value1 = value1; this->value2 = 0; }
	
	int getKey() { return key; }

	std::string getValue1() { return value1; }
	
	int getValue2() { return value2; }

	void increment(void) { ++value2; }

	private:

	int key;
	std::string value1;
	int value2;
};


class HashMap
{
	public:

	HashMap(unsigned size)
	{
		this->size = size;
		table = new HashEntry*[size];

		for (unsigned i = 0; i < size; ++i)
			table[i] = nullptr;
	}

	std::string get1(unsigned key)
	{
		unsigned bucket = getBucket(key);

		if (bucket != -1)
			return table[bucket]->getValue1();

		return std::string("NOT_FOUND");
	}

	int get2(unsigned key)
	{
		unsigned bucket = getBucket(key);

		if (bucket != -1)
		{
			return table[bucket]->getValue2();
		}

		return -1;
	}

	void increment2(unsigned key)
	{
		unsigned bucket = getBucket(key);

		if (bucket != -1)
			table[bucket]->increment();
	}

	void put(unsigned key, std::string value1)
	{
		unsigned start = (key % size);
		unsigned bucket = start;

		while (table[bucket] != nullptr)
		{
			bucket = (bucket + 1) % size;
			if (bucket == start)
				break;
		}
			
		if (table[bucket] != nullptr)
		{
			std::cout << "Hash table is full!\n";
			exit(-1);
		}

		table[bucket] = new HashEntry(key, value1);
		entry temp;
		temp.key = key;
		temp.name = value1;
		list.emplace_back(temp);
	}

	~HashMap()
	{
		for (unsigned i = 0; i < size; ++i)
			if (table[i] != nullptr)
				delete table[i];

		delete[] table;
	}

	std::vector<entry> getList(void) { return list; }

	void dumpList(void)
	{
		for (unsigned i = 0; i < list.size(); ++i)
		{
			std::cout << i << " key=" << list[i].key << "  name=" << list[i].name << "\n";
		}
	}

	private:

	HashEntry **table;
	unsigned size;
	std::vector<entry> list;

	int getBucket(unsigned key)
	{

		unsigned start = (key % size);
		unsigned bucket = start;

		while (table[bucket] != nullptr && table[bucket]->getKey() != key)
		{
			bucket = (bucket + 1) % size;
			if (bucket == start)
				break;
		}

		if (table[bucket] != nullptr && table[bucket]->getKey() == key)
			return (int)bucket;

		return -1;
	}
};

int main(int argc, char *argv[])
{
	FILETIME creationTime, exitTime, kernelTime, userTime;
	LPFILETIME creation = &creationTime, exitT = &exitTime, kernel = &kernelTime, user = &userTime;
	ULONGLONG u1, u2;
	HANDLE myProcess = GetCurrentProcess();
	BOOL gotTime1, gotTime2;
	DWORD failReason;
	SYSTEMTIME loct;
	double fStartTime, fEndTime, workTime, workUserTime;


	u1 = getTime64(user);
	GetLocalTime(&loct);
	fStartTime = loct.wHour * 3600 + loct.wMinute * 60 + loct.wSecond + (loct.wMilliseconds / 1000.0);

	std::ifstream colorFile(COLOR_FILE);
	if (!colorFile.is_open())
	{
		std::cout << "Failed to open file " << COLOR_FILE << ".\n";
		exit(-1);
	}

	HashMap copyrightMap(HASH_SIZE);
	std::string line;
	std::string name;
	unsigned short r, g, b;
	unsigned key;
	while (getline(colorFile, line))
	{
		std::stringstream buffer(line);
		buffer >> r >> g >> b >> name;
		key = ((((r << 8) | g) << 8) | b);
		copyrightMap.put(key, name);
	}


	std::ifstream bmpFile(PICTURE_FILE, std::ios::binary);
	if (!bmpFile.is_open())
	{
		std::cout << "Failed to open file " << PICTURE_FILE << ".\n";
		exit(-1);
	}

	unsigned short int bitsPerPixel;
	bmpFile.seekg(28);
	bmpFile.read((char *) &bitsPerPixel, sizeof(bitsPerPixel));

	if (bitsPerPixel != 24)
	{
		std::cout << "Error bmp file must be 24 bits per pixel.\n";
		exit(-1);
	}

	unsigned width;
	unsigned height;
	unsigned bitmapStart;

	bmpFile.seekg(18);
	bmpFile.read((char *)&width, sizeof(width));
	bmpFile.seekg(22);
	bmpFile.read((char *)&height, sizeof(height));
	bmpFile.seekg(10);
	bmpFile.read((char *)&bitmapStart, sizeof(bitmapStart));


	unsigned padding = (width * 3) % 4;
	char red, green, blue;
	bmpFile.seekg(bitmapStart);
	unsigned unfound = 0;
	for (unsigned y = 0; y < height; ++y)
	{
		for (unsigned x = 0; x < width; ++x)
		{
			bmpFile.read(&blue, 1);
			bmpFile.read(&green, 1);
			bmpFile.read(&red, 1);
			key = ((((red << 8) | green) << 8) | blue);

			if (copyrightMap.get2(key) != -1)
			{
				copyrightMap.increment2(key);
			}
				
			else
				++unfound;
		}
		bmpFile.seekg(padding, std::ios_base::cur);
	}

	bmpFile.close();

	std::vector<entry> list = copyrightMap.getList();

	for (unsigned i = 0, size = list.size(); i < size; ++i)
	{
		key = list[i].key;
		std::string name = list[i].name;
		std::cout << "(R=" << ((key & 0xFF0000) >> 16);
		std::cout << ", G=" << ((key & 0xFF00) >> 8);
		std::cout << ", B=" << (key & 0xFF);
		std::cout << ")   " << copyrightMap.get1(key) << "=" << copyrightMap.get2(key) << "   " << name << "\n";
	}

	std::cout << "Colors not in file=" << unfound << "\n";

	copyrightMap.dumpList();

	gotTime2 = GetProcessTimes(myProcess, creation, exitT, kernel, user);
	if (!gotTime2) {
		failReason = GetLastError();
		std::cout << "GetProcessTimes failed, Failure reason:" << failReason << std::endl;
		exit(-1);
	}

	GetLocalTime(&loct);
	fEndTime = loct.wHour * 3600 + loct.wMinute * 60 + loct.wSecond + (loct.wMilliseconds / 1000.0);
	u2 = getTime64(user);
	workUserTime = (u2 - u1) * 100.0 / 1000000000.0; // = usertime
	workTime = fEndTime - fStartTime; // = system CPU time



}

ULONGLONG getTime64(LPFILETIME a) {
	ULARGE_INTEGER work;
	work.HighPart = a->dwHighDateTime;
	work.LowPart = a->dwLowDateTime;
	return work.QuadPart;
}
