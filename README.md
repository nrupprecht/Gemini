# Gemini
The Gemini C++ image and plotting library.

Rendering and plotting is an interesting project, and there isn't really a go-to C++ plotting library like matplot lib for python,
so I thought this would be a nice and perhaps even useful project to develop.

Gemini is based around a relational model, where you specify how the sub-object that make up a plot
(like shapes, sub-canvases, textboxes, etc.) relate (spatially) to one another. In general, I want to 
have a system where you specify the important relationships between parts of your image or plot, and
let the image solve for where everything should be.

Since no plotting software is complete without the ability to render text (e.g. for labels, axis ticks, titles, etc.),
I've implemented objects to read font specifications from files in TrueType format and raster text to bitmaps
based on that information. This was actually the most difficult part, since fonts are fairly complex. I haven't 
implemented all the TrueType standard, but have enough that it can render very recognizable text to images,
and uses the basic spacing and character placement (no kerning, gpos, or instructing fonts yet).

##Project Layout
Overview of the project layout as it is at this time. This basically the folder directory. There are generally files 
in (some) directories that contain other directories. For example, the *gemini/core* directory contains bothe the 
*gemini/core/shapes* directory as well as the file *Bitmap.h*.

* **EasyBMP** - The excellent EasyBMP project (lightly modified), which is a great library for making Bitmaps in C++ (see http://easybmp.sourceforge.net/).
* **gemini** - Contains all the gemini project
  * **core** - Contains basic objects, like Bitmap, Canvas, and various coordinate and utility structures.
    * **shapes** - Contains
  * **plot** - Contains classes related to plotting. This uses the core gemini definitions, creating an interface that can be used to make images containing line, scatter, and error plots.
  * **text** - Contains text and TrueType related functions and objects. 
