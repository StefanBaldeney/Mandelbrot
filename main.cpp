//  main.cpp / MicroMandelbrot / Created by Stefan Brandt on 18.08.20.
//  Copyright © 2020 Stefan Brandt. All rights reserved.

#include <iostream>
#include <string>
#include <complex>
#include <fstream>

// nicht ganz so wichtig
#include <chrono>

// umschreiben, falls es auf dem Raspberry pi nicht geht
#include <atomic>
#include <thread>

using namespace std;

// Mandelbrot **********************************************

class PixelData
{
public:
    int width;
    int height;
    short* pPixels;

    PixelData(int width, int height)
    {
        this->width = width;
        this->height = height;
        pPixels = new short[width * height];
        for(int i=0;i<width*height;i++)
        {
            pPixels[i] = 0;
        }
    }

    ~PixelData()
    {
        // delete[] pPixels;
        // cout << "Destruktor von PixelData!" << endl;
    }
};

class Mandelbrot
{
public:
    Mandelbrot(int maxIter);
    ~Mandelbrot();

    int maxIter = 255;

    float limit=2;

    float realMin;
    float realMax;
    float imagMin;
    float imagMax;

    int init(float realMin, float realMax, float imagMin, float imagMax);

    PixelData Compute(int width, int height);

    static void calc(int y, int width, float reamMin, float imagMin, float diffX, float diffY, PixelData p);
    
protected:
    short eval(complex<float> c);
};

Mandelbrot::Mandelbrot(int maxIter)
{
    this->maxIter = maxIter;
}

Mandelbrot::~Mandelbrot()
{
}

void Mandelbrot::calc(int y, int width, float realMin, float imagMin, float diffX, float diffY, PixelData p)
{
    // cout << "calcUpper" << endl;
    
    for (int x = 0; x < width; x++)
    {
        auto c = complex<float>(realMin + x * diffX, imagMin + y* diffY);
        auto z = complex<float>(0, 0);

        short result = 0;
        for (short i = 0; i < 512; i++)
        {
            z = z * z + c;
            result = i;
            
            if (abs(z) > 2)
                break;
        }
        
        p.pPixels[x+y*width] = result;
    }
}

int Mandelbrot::init(float realMin, float realMax, float imagMin, float imagMax)
{
    this->realMin = realMin;
    this->realMax = realMax;
    this->imagMin = imagMin;
    this->imagMax = imagMax;
    return 0;
}

PixelData Mandelbrot::Compute(int width, int height)
{
 PixelData p(width, height);
        
 float diffX = (realMax - realMin) / p.width;
 float diffY = (imagMax - imagMin) / p.height;

 chrono::system_clock::time_point start = chrono::system_clock::now();
    
//    for (int y = 0; y < height; y+=2)
//     {
//         thread t1(calc, y, width, realMin, imagMin, diffX, diffY, p);
//         thread t2(calc, y + 1, width, realMin, imagMin, diffX, diffY, p);
//
//         t1.join();
//         t2.join();
//     }
    
 for (int y = 0; y < height; y+=1)
 {
     calc(y, width, realMin, imagMin, diffX, diffY, p);
 }

 chrono::system_clock::time_point finish = chrono::system_clock::now();

 chrono::duration<double> duration = finish - start;
      
 cout << "Duration in seconds: " << duration.count() << endl;
    
 return p;
}

// Ende Mandelbrot ******************************************

// BITMAP ***************************************************
class Bitmap
{
public:
    Bitmap();
    ~Bitmap();

    short Pixels[1024*768];
    void initialisiere(short pixels[1024*768]);
    
    void write(ofstream& fs, uint16_t data);
    void write(ofstream& fs, unsigned int data);
    void speichern(string dateiname);
    
};

union IntData {
    int i;
    char bytes[4];
};

union ShortData {
    int s;
    char bytes[2];
};

Bitmap::Bitmap(){}
Bitmap::~Bitmap(){}

void Bitmap::initialisiere(short pixels[1024 * 768])
{
    int c = 0;
        for(int y=0;y<768;y++)
            for (int x = 0; x < 1024; x++)
            {
                int index = c * 1024*768 + y * 1024 + x;
                Pixels[index] = (pixels[1024 * y + x]);
            }
}

void Bitmap::write(ofstream& fs, unsigned int i)
{
    IntData d;
    d.i = i;
    
    fs.put(d.bytes[0]);
    fs.put(d.bytes[1]);
    fs.put(d.bytes[2]);
    fs.put(d.bytes[3]);
}

void Bitmap::write(ofstream& fs, uint16_t data)
{
    ShortData d;
    d.s = data;

    fs.put(d.bytes[0]);
    fs.put(d.bytes[1]);
}

void Bitmap::speichern(string dateiname)
{
    // https://de.wikipedia.org/wiki/Windows_Bitmap
    
    // trunc = truncate = bestehende inhalte abschneiden
    // ios::binary, sonst werden eventuell Zeilenwechsel wie 0x0D0A statt 0x0A eingefügt
    ofstream bm (dateiname, ios::out | ios::trunc | ios::binary);
 
    bm << "BM"; // BITMAPFILEHEADER
    
    write(bm,(uint32_t) 54+3*1024*768);
    write(bm,(uint32_t) 0); // reserved
    write(bm, (unsigned)54); // btOffBits
    
    // BITMAPINFOHEADER
    write(bm, (uint32_t)40); // length in bytes
    write(bm, (uint32_t)1024); // width in pixels
    write(bm, (uint32_t)768); // height in pixels
    write(bm, (uint16_t)1); // 1 planes
    write(bm, (uint16_t)24); // bit count
    write(bm, (uint32_t)0); // uncompressed
    write(bm, (uint32_t)(1024*768*3)); // 0 oder size in bytes
    write(bm, (uint32_t)0); // x ppm
    write(bm, (uint32_t)0); // y ppm
    write(bm, (uint32_t)0); // biClrUsed
    write(bm, (uint32_t)0);  // biClrImportant

    // Pixeldaten
    for(int y=0;y<768;y++)
        for(int x=0;x<1024;x++)
        {
            short s = Pixels[y * 1024 + x];
            bm.put((s % 128)); // blue
            bm.put(((s >> 3) % 128)); // green
            bm.put(((s >> 5) % 128)); // red
        }
    bm.close();
}

// BITMAP ****************************************************

int main(int argc, const char * argv[]) {

    // program.exe -1.0 1.0 -0.5 0.5 "Benoit.bmp"
    
    cout << "creating Mandelbrot bitmap\n";

    Mandelbrot m(127);
    m.init(-2.00, 1.00, -1.0, 1.0);
    
    auto pixelData = m.Compute(1024, 768);
        
    // genaue Uhrzeit
    // berechne Mandelbrot
    // Aufruf mit Pfad/Filename zu der Bitmap-Datei, die erstellt werden soll

    Bitmap bitmap;
    bitmap.initialisiere(pixelData.pPixels);
    
    
    // todo hier pfad ändern!
    bitmap.speichern("/Users/stefanbrandt/Benoît.bmp");
    
    cout << "fertig mit Mandelbrot!" << endl;
    
    return 0;
}
