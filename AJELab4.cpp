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

ULONGLONG getTime64(LPFILETIME a);

class colorhash
{
public:
	colorhash(unsigned size)
	{
		this->size = size;
		table = new color*[size];

		for (unsigned i = 0; i < size; ++i)
			table[i] = nullptr;
	}
	~colorhash()
	{
		for (unsigned i = 0; i < size; ++i)
			if (table[i] != nullptr)
				delete table[i];

		delete table;
	}
	unsigned getKey(unsigned char red, unsigned char green, unsigned char blue)
	{
		return ((((red << 8) | green) << 8) | blue);
	}
	void put(unsigned key, std::string name)
	{
		color *insert = new color;
		insert->key = key;
		insert->name = name;
		insert->count = 0;

		unsigned bucket = key % size;
		unsigned start = bucket;

		while (true)
		{
			++comparisons;
			if (table[bucket] == nullptr)
			{
				table[bucket] = insert;
				buckets.emplace_back(bucket);
				break;
			}

			++bucket;

			if (bucket == size)
				bucket = 0;

			if (bucket == start)
			{
				std::cout << "Error hash table full!!\n";
				exit(-1);
			}
		}
	}

	bool check(unsigned key)
	{
		unsigned bucket = key % size;

		while (true)
		{
			++comparisons;
			if (table[bucket] == nullptr)
				return false;
			else if (table[bucket]->key == key)
				return true;

			++bucket;

			if (bucket == size)
				bucket = 0;
		}
	}
	void increment(unsigned key)
	{
		unsigned bucket = key % size;

		while (true)
		{
			++comparisons;
			if (table[bucket] != nullptr)
				if (table[bucket]->key == key)
				{
					++table[bucket]->count;
					return;
				}

			++bucket;

			if (bucket == size)
				bucket = 0;
		}
	}
	void print(void)
	{
		for (unsigned i = 0, max = buckets.size(), bucket; i < max; ++i)
		{
			bucket = buckets[i];

			std::cout << table[bucket]->name << " = " << table[bucket]->count << "\n";
		}

		std::cout << "\nComparisions = " << comparisons << "\n";
	}

private:

	struct color
	{
		unsigned key;
		std::string name;
		unsigned count;
	};
	color **table;
	unsigned size;
	std::vector<unsigned> buckets;
	unsigned comparisons = 0;
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
	struct color
	{
		std::string name;
		unsigned count;
	};
	colorhash hash(HASH_SIZE);

	gotTime1 = GetProcessTimes(myProcess, creation, exitT, kernel, user);
	u1 = getTime64(user);
	GetLocalTime(&loct);
	fStartTime = loct.wHour * 3600 + loct.wMinute * 60 + loct.wSecond + (loct.wMilliseconds / 1000.0);

	std::ifstream colorFile(COLOR_FILE);
	if (!colorFile.is_open())
	{
		std::cout << "Failed to open file " << COLOR_FILE << ".\n";
		exit(-1);
	}

	std::string line;
	std::string name;
	std::string temp;
	unsigned short r, g, b;
	unsigned key;
	while (getline(colorFile, line))
	{
		std::stringstream buffer(line);
		buffer >> r >> g >> b >> name;
		while (buffer >> temp)
		{
			name += (" " + temp);
		}

		key = hash.getKey(r, g, b);
		hash.put(key, name);
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
	unsigned char red, green, blue;
	bmpFile.seekg(bitmapStart);
	unsigned unfound = 0;
	for (unsigned y = 0; y < height; ++y)
	{
		for (unsigned x = 0; x < width; ++x)
		{
			bmpFile.read((char *)&blue, 1);
			bmpFile.read((char *)&green, 1);
			bmpFile.read((char *)&red, 1);
			key = hash.getKey(red, green, blue);

			if (hash.check(key))
				hash.increment(key);
			else
				++unfound;
		}
		bmpFile.seekg(padding, std::ios_base::cur);
	}

	bmpFile.close();

	std::cout << "Colors not in file=" << unfound << "\n";

	hash.print();

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

	std::cout << "search took " << workTime << " seconds of system CPU time and " << workUserTime << " seconds of user CPU time.\n\n\n";
}

ULONGLONG getTime64(LPFILETIME a) {
	ULARGE_INTEGER work;
	work.HighPart = a->dwHighDateTime;
	work.LowPart = a->dwLowDateTime;
	return work.QuadPart;
}
