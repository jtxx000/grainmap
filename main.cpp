#include <juce/juce.h>
#include <stdio.h>

using namespace juce;

int main() {
  ScopedJuceInitialiser_NonGUI initJuce;
  int width=800, height=600;
  Image img(Image::RGB, width, height, true, Image::SoftwareImage);
  // img.setPixelAt(x,y,color)
  //Image::BitmapData imgData(img, 0, 0, width, height, true);
  for (int y=0; y<height; y++) {
    for (int x=0; x<width; x++) {
      img.setPixelAt(x,y,Colour(0,100,255));
      /*uint8* pixel = imgData.getPixelPointer(x,y);
      pixel[PixelRGB::indexR] = 0;
      pixel[PixelRGB::indexG] = 100;
      pixel[PixelRGB::indexB] = 255;*/
    }
  }
  File f("out.png");
  if (f.exists()) f.deleteFile();
  FileOutputStream os(f);
  PNGImageFormat().writeImageToStream(img, os);
}

//clas

//START_JUCE_APPLICATION
