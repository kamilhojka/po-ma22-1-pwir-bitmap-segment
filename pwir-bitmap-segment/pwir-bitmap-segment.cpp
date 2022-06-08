// pwir-bitmap-segment.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <fstream>
#include <iostream>
#include <windows.h>
#include <chrono>
#include <thread>
#include <vector>
#include <mutex>
#include <omp.h>
using namespace std;

#define MAX_POINTS_SIZE 100
#define MAX_BITMAP_SIZE 500

mutex mtx;

struct Point {
	int x;
	int y;
	Point() {};
	Point(int x, int y)
	{
		this->x = x;
		this->y = y;
	}
};

struct BMP {
	unsigned char infoHeader[54];
	int width;
	int height;
	int paddingRow;
	unsigned char** data;

	BMP(const char* fname) {
		FILE* fpoint;
		errno_t err = fopen_s(&fpoint, fname, "rb");
		if (err != 0) {
			throw "Plik o tej nazwie nie istnieje";
		}
		fread(infoHeader, sizeof(unsigned char), 54, fpoint);
		width = *(int*)&infoHeader[18];
		height = *(int*)&infoHeader[22];
		paddingRow = (width * 3 + 3) & (~3);
		data = new unsigned char* [height];
		for (int i = 0; i < height; i++)
		{
			data[i] = new unsigned char[paddingRow];
			fread(data[i], sizeof(unsigned char), paddingRow, fpoint);
		}
		fclose(fpoint);
	}

	~BMP() {
		for (int i = 0; i < height; i++) {
			delete[] data[i];
		}
		delete[] data;
	}

	void write(const char* fname) {
		FILE* fpoint;
		errno_t err = fopen_s(&fpoint, fname, "wb");
		if (err != 0) {
			throw "Plik o tej nazwie nie istnieje";
		}
		fwrite(infoHeader, sizeof(unsigned char), 54, fpoint);
		for (int i = 0; i < height; i++) {
			fwrite(data[i], sizeof(unsigned char), paddingRow, fpoint);
		}
		fclose(fpoint);
	}
};

void ShowIntroInformation(HANDLE hConsole);
void SetNumberOfPoints(HANDLE hConsole, int& number);
void SetThicknessOfLines(HANDLE hConsole, int& thicknessOfLines);
void SetDelay(HANDLE hConsole, int& delay);
void SetRandomOption(HANDLE hConsole, int& randomOption);
void SetPointValues(HANDLE hConsole, Point& point, int number);
void RunDrawingLines(HANDLE hConsole, Point points[MAX_POINTS_SIZE], int numberOfPoints, int thicknessOfLines, int delay);
void RunDrawingLinesParralelOpenMP(HANDLE hConsole, Point points[MAX_POINTS_SIZE], int numberOfPoints, int thicknessOfLines, int delay);
void RunDrawingLinesParralelThread(HANDLE hConsole, Point points[MAX_POINTS_SIZE], int numberOfPoints, int thicknessOfLines, int delay);

int main()
{
	setlocale(LC_CTYPE, "Polish");
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	int numberOfPoints, thicknessOfLines, randomOption, delay = 0;
	Point points[MAX_POINTS_SIZE];
	ShowIntroInformation(hConsole);
	SetNumberOfPoints(hConsole, numberOfPoints);
	SetThicknessOfLines(hConsole, thicknessOfLines);
	SetDelay(hConsole, delay);
	SetRandomOption(hConsole, randomOption);
	if (randomOption) {
		srand(time(NULL));
		for (int i = 0; i < numberOfPoints; i++)
		{
			points[i] = Point(
				rand() % (MAX_BITMAP_SIZE - thicknessOfLines * 2) + thicknessOfLines * 2, 
				rand() % (MAX_BITMAP_SIZE - thicknessOfLines * 2) + thicknessOfLines * 2
			);
		}
	}
	else {
		for (int i = 0; i < numberOfPoints; i++)
		{
			SetPointValues(hConsole, points[i], i + 1);
		}
	}
	RunDrawingLines(hConsole, points, numberOfPoints, thicknessOfLines, delay);
	RunDrawingLinesParralelThread(hConsole, points, numberOfPoints, thicknessOfLines, delay);
	RunDrawingLinesParralelOpenMP(hConsole, points, numberOfPoints, thicknessOfLines, delay);
}

void ShowIntroInformation(HANDLE hConsole)
{
	SetConsoleTextAttribute(hConsole, 11);
	for (int i = 0; i < 70; i++) cout << '*';
	SetConsoleTextAttribute(hConsole, 3);
	cout << "\n\n  PROGRAMOWANIE WSPÓ£BIE¯NE I ROZPROSZONE 21/22L\n  Bitmap [500x500] - Rysowanie odcinka\n  Autor programu: ";
	SetConsoleTextAttribute(hConsole, 15);
	cout << "Kamil Hojka -- 97632\n\n";
	SetConsoleTextAttribute(hConsole, 11);
	for (int i = 0; i < 70; i++) cout << '*';
	cout << "\n";
	SetConsoleTextAttribute(hConsole, 15);
}

void SetNumberOfPoints(HANDLE hConsole, int& number)
{
	SetConsoleTextAttribute(hConsole, 14);
	cout << "\n -> Podaj liczbê punktów ";
	SetConsoleTextAttribute(hConsole, 4);
	cout << "[max = 100]: ";
	while (true) {
		SetConsoleTextAttribute(hConsole, 15);
		cin >> number;
		if (cin.good() && (number >= 0 && number <= 100)) break;
		SetConsoleTextAttribute(hConsole, 4);
		cout << "    ! Wartoœæ musi byæ liczb¹ z przedzia³u od 0 do 100\n";
		SetConsoleTextAttribute(hConsole, 15);
		cin.clear();
		cin.ignore();
	}
}

void SetThicknessOfLines(HANDLE hConsole, int& thicknessOfLines)
{
	SetConsoleTextAttribute(hConsole, 14);
	cout << "\n -> Podaj gruboœæ linii: ";
	while (true) {
		SetConsoleTextAttribute(hConsole, 15);
		cin >> thicknessOfLines;
		if (cin.good())
		{
			if (thicknessOfLines <= 0) thicknessOfLines = 1;
			break;
		}
		SetConsoleTextAttribute(hConsole, 4);
		cout << "    ! Wartoœæ gruboœci linii musi byæ liczb¹\n";
		SetConsoleTextAttribute(hConsole, 15);
		cin.clear();
		cin.ignore();
	}
}

void SetDelay(HANDLE hConsole, int& delay)
{
	SetConsoleTextAttribute(hConsole, 14);
	cout << "\n -> Podaj opóŸnienie [ms]: ";
	while (true) {
		SetConsoleTextAttribute(hConsole, 15);
		cin >> delay;
		if (cin.good())
		{
			if (delay < 0) delay = 0;
			break;
		}
		SetConsoleTextAttribute(hConsole, 4);
		cout << "    ! Wartoœæ opóŸnienia musi byæ liczb¹\n";
		SetConsoleTextAttribute(hConsole, 15);
		cin.clear();
		cin.ignore();
	}
}

void SetRandomOption(HANDLE hConsole, int& randomOption)
{
	SetConsoleTextAttribute(hConsole, 14);
	cout << "\n -> Czy wylosowaæ punkty losowo? [1/0]: ";
	while (true) {
		SetConsoleTextAttribute(hConsole, 15);
		cin >> randomOption;
		if (cin.good() && (randomOption == 0 || randomOption == 1)) break;
		SetConsoleTextAttribute(hConsole, 4);
		cout << "    ! Wartoœæ musi byæ liczb¹ 1 lub 0\n";
		SetConsoleTextAttribute(hConsole, 15);
		cin.clear();
		cin.ignore();
	}
}

void SetPointValues(HANDLE hConsole, Point& point, int number)
{
	SetConsoleTextAttribute(hConsole, 14);
	cout << "\n -> Podaj wspó³rzêdne punktu " << number;
	SetConsoleTextAttribute(hConsole, 4);
	cout << "[min = 0, max = 500]";
	while (true) {
		SetConsoleTextAttribute(hConsole, 14);
		cout << "\n --> x: ";
		SetConsoleTextAttribute(hConsole, 15);
		cin >> point.x;
		if (cin.good() && (point.x >= 0 && point.x <= 500)) break;
		SetConsoleTextAttribute(hConsole, 4);
		cout << "    ! Wartoœæ wspó³rzêdnej x musi byæ liczb¹ w przedziale od 0 do 500\n";
		SetConsoleTextAttribute(hConsole, 15);
		cin.clear();
		cin.ignore();
	}
	while (true) {
		SetConsoleTextAttribute(hConsole, 14);
		cout << " --> y: ";
		SetConsoleTextAttribute(hConsole, 15);
		cin >> point.y;
		if (cin.good() && (point.y >= 0 && point.y <= 500)) break;
		SetConsoleTextAttribute(hConsole, 4);
		cout << "    ! Wartoœæ wspó³rzêdnej y musi byæ liczb¹ w przedziale od 0 do 500\n";
		SetConsoleTextAttribute(hConsole, 15);
		cin.clear();
		cin.ignore();
	}
}

void DrawLine(BMP& bitmap, Point start, Point end) 
{
	int index;
	double a = (double)(end.y - start.y) / (double)(end.x - start.x);
	double b = (double)(start.y - a * start.x);

	if (abs(start.x - end.x) >= abs(start.y - end.y)) 
	{
		if (start.x > end.x) 
		{
			swap(start, end);
		}
		for (int x = start.x * 3; x < end.x * 3; x += 3)
		{
			index = round(a * x / 3 + b);
			if (index >= 0 && index < bitmap.height && x >= 0 && x < 3 * bitmap.width) 
			{
				bitmap.data[index][x] = 0;
				bitmap.data[index][x + 1] = 0;
				bitmap.data[index][x + 2] = 0;
			}
		}
	}
	else 
	{
		if (start.y > end.y) 
		{
			swap(start, end);
		}
		for (int y = start.y; y < end.y; y++)
		{
			index = 3 * round((y - b) / a);
			if (index >= 0 && index < 3 * bitmap.width && y >= 0 && y < bitmap.height) 
			{
				bitmap.data[y][index] = 0;
				bitmap.data[y][index + 1] = 0;
				bitmap.data[y][index + 2] = 0;
			}
		}
	}
}

void DrawLineWithThickness(BMP& bitmap, Point start, Point end, int thicknessOfLines, int delay) {
	this_thread::sleep_for(std::chrono::milliseconds(delay));
	Point v(end.y - start.y, -(end.x - start.x));
	double l = sqrt(v.x * v.x + v.y * v.y);
	double x = (double)v.x / l;
	double y = (double)v.y / l;

	for (int i = -thicknessOfLines; i <= thicknessOfLines; i++) {
		int x1 = round(start.x + x * (double)i / 2);
		int y1 = round(start.y + y * (double)i / 2);

		int x2 = round(end.x + x * (double)i / 2);
		int y2 = round(end.y + y * (double)i / 2);
		DrawLine(bitmap, Point(x1, y1), Point(x2, y2));
	}
	if (abs(start.x - end.x) >= abs(start.y - end.y)) {
		for (int i = -thicknessOfLines; i < thicknessOfLines; i++) {
			int x1 = round(start.x + x * (double)i / 2);
			int y1 = round(start.y + 1 + y * (double)i / 2);

			int x2 = round(end.x + x * (double)i / 2);
			int y2 = round(end.y + 1 + y * (double)i / 2);
			DrawLine(bitmap, Point(x1, y1), Point(x2, y2));
		}
	}
	else {
		for (int i = -thicknessOfLines; i < thicknessOfLines; i++) {
			int x1 = round(start.x + 1 + x * (double)i / 2);
			int y1 = round(start.y + y * (double)i / 2);

			int x2 = round(end.x + 1 + x * (double)i / 2);
			int y2 = round(end.y + y * (double)i / 2);
			DrawLine(bitmap, Point(x1, y1), Point(x2, y2));
		}
	}
}

void RunDrawingLines(HANDLE hConsole, Point points[MAX_POINTS_SIZE], int numberOfPoints, int thicknessOfLines, int delay)
{
	cout << "\n\n";
	SetConsoleTextAttribute(hConsole, 11);
	for (int i = 0; i < 70; i++) cout << '*';
	SetConsoleTextAttribute(hConsole, 3);
	cout << "\n ---> Sekwencyjne rysowanie odcinka - Bitmapa [500x500]\n";
	SetConsoleTextAttribute(hConsole, 15);
	const char* fileName = "plain.bmp";
	try
	{
		BMP bitmap(fileName);
		auto begin = chrono::high_resolution_clock::now();
		for (int i = 0; i < numberOfPoints - 1; i++)
		{
			DrawLineWithThickness(bitmap, points[i], points[i + 1], thicknessOfLines, delay);
		}
		bitmap.write("output_sequentially.bmp");
		auto end = chrono::high_resolution_clock::now();
		auto elapsed = chrono::duration_cast<chrono::milliseconds>(end - begin).count();
		cout << "\n Zmierzony czas: " << elapsed << " ms\n";
	}
	catch (const char* ex)
	{
		SetConsoleTextAttribute(hConsole, 4);
		cout << "\n B³¹d: ";
		SetConsoleTextAttribute(hConsole, 14);
		cout << ex << " (" << fileName << ")\n";
		SetConsoleTextAttribute(hConsole, 15);
	}
}


void DrawLineWithMutex(BMP& bitmap, Point start, Point end)
{
	int index;
	double a = (double)(end.y - start.y) / (double)(end.x - start.x);
	double b = (double)(start.y - a * start.x);

	if (abs(start.x - end.x) >= abs(start.y - end.y))
	{
		if (start.x > end.x)
		{
			swap(start, end);
		}
		for (int x = start.x * 3; x < end.x * 3; x += 3)
		{
			index = round(a * x / 3 + b);
			if (index >= 0 && index < bitmap.height && x >= 0 && x < 3 * bitmap.width)
			{
				mtx.lock();
				bitmap.data[index][x] = 0;
				bitmap.data[index][x + 1] = 0;
				bitmap.data[index][x + 2] = 0;
				mtx.unlock();
			}
		}
	}
	else
	{
		if (start.y > end.y)
		{
			swap(start, end);
		}
		for (int y = start.y; y < end.y; y++)
		{
			index = 3 * round((y - b) / a);
			if (index >= 0 && index < 3 * bitmap.width && y >= 0 && y < bitmap.height)
			{
				mtx.lock();
				bitmap.data[y][index] = 0;
				bitmap.data[y][index + 1] = 0;
				bitmap.data[y][index + 2] = 0;
				mtx.unlock();
			}
		}
	}
}

void DrawLineWithThicknessWithMutex(BMP& bitmap, Point start, Point end, int thicknessOfLines, int delay) {
	this_thread::sleep_for(std::chrono::milliseconds(delay));
	Point v(end.y - start.y, -(end.x - start.x));
	double l = sqrt(v.x * v.x + v.y * v.y);
	double x = (double)v.x / l;
	double y = (double)v.y / l;

	for (int i = -thicknessOfLines; i <= thicknessOfLines; i++) {
		int x1 = round(start.x + x * (double)i / 2);
		int y1 = round(start.y + y * (double)i / 2);

		int x2 = round(end.x + x * (double)i / 2);
		int y2 = round(end.y + y * (double)i / 2);
		DrawLineWithMutex(bitmap, Point(x1, y1), Point(x2, y2));
	}
	if (abs(start.x - end.x) >= abs(start.y - end.y)) {
		for (int i = -thicknessOfLines; i < thicknessOfLines; i++) {
			int x1 = round(start.x + x * (double)i / 2);
			int y1 = round(start.y + 1 + y * (double)i / 2);

			int x2 = round(end.x + x * (double)i / 2);
			int y2 = round(end.y + 1 + y * (double)i / 2);
			DrawLineWithMutex(bitmap, Point(x1, y1), Point(x2, y2));
		}
	}
	else {
		for (int i = -thicknessOfLines; i < thicknessOfLines; i++) {
			int x1 = round(start.x + 1 + x * (double)i / 2);
			int y1 = round(start.y + y * (double)i / 2);

			int x2 = round(end.x + 1 + x * (double)i / 2);
			int y2 = round(end.y + y * (double)i / 2);
			DrawLineWithMutex(bitmap, Point(x1, y1), Point(x2, y2));
		}
	}
}

void RunDrawingLinesParralelThread(HANDLE hConsole, Point points[MAX_POINTS_SIZE], int numberOfPoints, int thicknessOfLines, int delay)
{
	cout << "\n\n";
	SetConsoleTextAttribute(hConsole, 11);
	for (int i = 0; i < 70; i++) cout << '*';
	SetConsoleTextAttribute(hConsole, 3);
	cout << "\n ---> Równoleg³e rysowanie odcinka za pomoc¹ thread - Bitmapa [500x500]\n";
	SetConsoleTextAttribute(hConsole, 15);
	const char* fileName = "plain.bmp";
	try
	{
		BMP bitmap(fileName);
		auto begin = chrono::high_resolution_clock::now();
		thread threads[MAX_BITMAP_SIZE];
		for (int i = 0; i < numberOfPoints - 1; i++)
		{
			threads[i] = thread(DrawLineWithThicknessWithMutex, ref(bitmap), points[i], points[i + 1], thicknessOfLines, delay);
		}
		for (int i = 0; i < numberOfPoints - 1; i++)
		{
			threads[i].join();
		}
		bitmap.write("output_parralel_thread.bmp");
		auto end = chrono::high_resolution_clock::now();
		auto elapsed = chrono::duration_cast<chrono::milliseconds>(end - begin).count();
		cout << "\n Zmierzony czas: " << elapsed << " ms\n";
	}
	catch (const char* ex)
	{
		SetConsoleTextAttribute(hConsole, 4);
		cout << "\n B³¹d: ";
		SetConsoleTextAttribute(hConsole, 14);
		cout << ex << " (" << fileName << ")\n";
		SetConsoleTextAttribute(hConsole, 15);
	}
}

void RunDrawingLinesParralelOpenMP(HANDLE hConsole, Point points[MAX_POINTS_SIZE], int numberOfPoints, int thicknessOfLines, int delay)
{
	cout << "\n\n";
	SetConsoleTextAttribute(hConsole, 11);
	for (int i = 0; i < 70; i++) cout << '*';
	SetConsoleTextAttribute(hConsole, 3);
	cout << "\n ---> Równoleg³e rysowanie odcinka za pomoc¹ OpenMP - Bitmapa [500x500]\n";
	SetConsoleTextAttribute(hConsole, 15);
	const char* fileName = "plain.bmp";
	try
	{
		BMP bitmap(fileName);
		auto begin = chrono::high_resolution_clock::now();
#pragma omp parallel for 
		for (int i = 0; i < numberOfPoints - 1; i++)
		{
			DrawLineWithThickness(bitmap, points[i], points[i + 1], thicknessOfLines, delay);
		}
		bitmap.write("output_parralel_openmp.bmp");
		auto end = chrono::high_resolution_clock::now();
		auto elapsed = chrono::duration_cast<chrono::milliseconds>(end - begin).count();
		cout << "\n Zmierzony czas: " << elapsed << " ms\n";
	}
	catch (const char* ex)
	{
		SetConsoleTextAttribute(hConsole, 4);
		cout << "\n B³¹d: ";
		SetConsoleTextAttribute(hConsole, 14);
		cout << ex << " (" << fileName << ")\n";
		SetConsoleTextAttribute(hConsole, 15);
	}
}