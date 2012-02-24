// artistactrl is a command line utility for writing data to USB connected
// LCD screens using the ArtistaSDK. You can buy screens and obtain the SDK
// from http://www.datadisplay-group.com/, and possibly others.
// Zoltan Olah (zol@me.com) released under the MIT license in 2007.

//original inclues for artista
#include <assert.h>
#include <vector>
#include <iostream>
#include <stm/artistadevice.hpp>

#ifdef _WIN32
#include <windows.h>                 // Needed only for Sleep().
#else
#include <time.h>
void Sleep (unsigned int msecs)
{
    struct timespec rqt = {msecs / 1000, (msecs % 1000) * 1000000};
    nanosleep (&rqt, 0);
}
#endif

//imagemagick includes
#include <Magick++.h>
using namespace Magick;

//other includes
#include <iostream>

#define SCREEN_TIMEOUT		5000 

#define UUID_SIZE 16

std::string UuidToString(stm::Uuid u)
{
	std::string ret = "";
	
	for (int i = 0;i < UUID_SIZE;i++) {
		unsigned char c;
		c = u.octet[i];
		
		if (c == 0)
			break;
			
		ret += c;		
	}
	
	return ret;
}

stm::Uuid StringToUuid(std::string s)
{
	unsigned char uuid[UUID_SIZE];
	
	for (int i = 0;i < UUID_SIZE;i++) {
		if (i < s.length())
			uuid[i] = s[i];
		else
			uuid[i] = 0;
	}

	return stm::Uuid(uuid);
}

// Convert to a raw 16bpp /565 bitmap format
int ConvertToBitmap(std::string filename, int width, int height, unsigned short* pic_cache)
{
	unsigned short *pic;
	unsigned short bgr16;
	
	pic = pic_cache;
	
	Image image;
	image.read(filename);
	
	//scale the image if need be
	if (image.baseRows() != height || image.baseColumns() != width) 
	{	
		Geometry geometry(width, height);
		geometry.aspect(true); //dont preserve aspect ratio
		
		image.sample(geometry);
	}
	
	for (int i = 0;i < height;i++)
	{
		for (int j = 0;j < width;j++)
		{
		    Color pixel_sample;
		    pixel_sample = image.pixelColor(j,i);

		    bgr16 = ((pixel_sample.blueQuantum() >> (3+(sizeof(Quantum)-1)*8)) |
		    	    ((pixel_sample.greenQuantum() >> (2+(sizeof(Quantum)-1)*8))<<5) |
		    	    ((pixel_sample.redQuantum() >> (3+(sizeof(Quantum)-1)*8))<<11) );
		    *pic =  bgr16;
		    pic++;
		}
	}
	
	return 0;
}

// Create a raw 16bpp /565 bitmap format with a number rendered on it
// as well as a string id
int MakeNumberBitmap(int number, std::string id, int width, int height, unsigned short* pic_cache)
{
	unsigned short *pic;
	unsigned short bgr16;
	
	pic = pic_cache;
	
	Image image(Geometry(width, height), "white");
	
	//convert number to string
	std::ostringstream o;	
	o << number;
	
	//render a number onto the screen	
	std::list<Magick::Drawable> drawList;
	drawList.push_back(DrawableStrokeColor("black"));
	drawList.push_back(DrawableStrokeWidth(4));
	drawList.push_back(DrawableFillColor("black"));
	drawList.push_back(DrawableText( 50, 100, "n:" + o.str() ));
	drawList.push_back(DrawableText( 50, 200, "id:" + id ));
	drawList.push_back(DrawablePointSize(100));	
	image.draw(drawList);
		
	for (int i = 0;i < height;i++)
	{
		for (int j = 0;j < width;j++)
		{
		    Color pixel_sample;
		    pixel_sample = image.pixelColor(j,i);

		    bgr16 = ((pixel_sample.blueQuantum() >> (3+(sizeof(Quantum)-1)*8)) |
		    	    ((pixel_sample.greenQuantum() >> (2+(sizeof(Quantum)-1)*8))<<5) |
		    	    ((pixel_sample.redQuantum() >> (3+(sizeof(Quantum)-1)*8))<<11) );
		    *pic =  bgr16;
		    pic++;
		}
	}
	
	return 0;
}

//List the attached screens
void ArtistaList()
{
	// Look for for all Artista devices connected and get their number.
	std::vector<stm::Device::Descriptor> artistaDescr;
	size_t numArtistas = stm::ArtistaDevice::enumerateAll (artistaDescr, stm::ArtistaDevice::LibUsbDriverAccess);
	std::cout << int (numArtistas) << " Artista devices found." << std::endl;

	// Open each Artista device.
	std::vector<stm::ArtistaDevice> artista (numArtistas);
	for (size_t i = 0; i < numArtistas; ++ i)
	{
		artista [i].setDescr (artistaDescr [i]);
		artista [i].open ();
	}

  // For each Artista device i
  for (size_t i = 0; i < numArtistas; ++ i)
  {
		// Get its type and geometry.
		int lines = artista [i].lines ();
		int columns = artista [i].columns ();
		
		// get string id
		stm::Uuid id = artista[i].displayId();
		std::string id_s = UuidToString(id);

		// Print the device description.
		std::cout << "Artista " << int (i + 1) << " is a ";
		artista [i].describe
		(
			std::cout,
			4 | stm::Device::VerboseProperties
		);
		
		artista[i].close();

		std::cout << "    Device Resolution: " << columns << "x" << lines << "\n";
		std::cout << "    Device UUID (as string): " << id_s << "\n";		
	}
}

// show an image on a given screen
void ArtistaShow(std::string screen, std::string filename)
{
	// Look for for all Artista devices connected and get their number.
	std::vector<stm::Device::Descriptor> artistaDescr;
	size_t numArtistas = stm::ArtistaDevice::enumerateAll (artistaDescr, stm::ArtistaDevice::LibUsbDriverAccess);
	
	int screenNum = atoi(screen.c_str());
	
	if (screenNum < 0 || screenNum >= numArtistas)
	{
		std::cout << "ERROR: screen " << screen << " does not exist!\n";
		exit(1);
	} 

	// Open given device
	stm::ArtistaDevice artista;
	artista.setDescr(artistaDescr[screenNum]);
	artista.open();
	artista.setDefaultTimeout(SCREEN_TIMEOUT);
	
	// Get geometry
	int pixels = artista.pixels ();
	int lines = artista.lines ();
	int columns = artista.columns ();
	
	// Load the image
	unsigned short *image = new unsigned short [pixels];            
	ConvertToBitmap(filename, columns, lines, image);

	// Switch the display power and backlight on.
	artista.setDisplayPower (true);
	artista.setBacklight (true);

	// Display the image.
	int err = artista.write (image, pixels * 2);
	artista.close();
	delete [] image;

	if (err == -1)
		std::cout << "ERROR: writing to screen " << screen << "\n";	
}

void ArtistaReset(std::string screen)
{
	// Look for for all Artista devices connected and get their number.
	std::vector<stm::Device::Descriptor> artistaDescr;
	size_t numArtistas = stm::ArtistaDevice::enumerateAll (artistaDescr, stm::ArtistaDevice::LibUsbDriverAccess);
	
	//reset all
	if (screen == "all")
	{
		for (int i = 0;i < numArtistas;i++)
		{
			//convert to int
			std::string si;
			std::ostringstream o;	
			o << i;
			
			ArtistaReset(o.str());
		}
	}
	
	//reset one screen
	int screenNum = atoi(screen.c_str());
	
	if (screenNum < 0 || screenNum >= numArtistas)
	{
		std::cout << "ERROR: screen " << screen << " does not exist!\n";
		exit(1);
	} 

	// Open given device
	stm::ArtistaDevice artista;
	artista.setDescr(artistaDescr[screenNum]);
	artista.open();
	artista.setDefaultTimeout(SCREEN_TIMEOUT);
	
	// Switch the display power and backlight on.
	artista.setDisplayPower(false);
	artista.setBacklight(false);
	artista.close();
}

//Number each attached screen
void ArtistaNumber()
{
	// Look for for all Artista devices connected and get their number.
	std::vector<stm::Device::Descriptor> artistaDescr;
	size_t numArtistas = stm::ArtistaDevice::enumerateAll (artistaDescr, stm::ArtistaDevice::LibUsbDriverAccess);
	//std::cout << int (numArtistas) << " Artista devices found." << std::endl;

	// Open each Artista device.
	std::vector<stm::ArtistaDevice> artista (numArtistas);
	for (size_t i = 0; i < numArtistas; ++ i)
	{
		artista [i].setDescr (artistaDescr [i]);
		artista [i].open ();
		artista [i].setDefaultTimeout(SCREEN_TIMEOUT);
	}

	// For each Artista device i
	for (size_t i = 0; i < numArtistas; ++ i)
	{
		// Get its type and geometry.
		int lines = artista [i].lines ();
		int columns = artista [i].columns ();
		int pixels = artista [i].pixels ();
		
		stm::Uuid id = artista[i].displayId();
		std::string id_s = UuidToString(id);
	
		// Create the image
		unsigned short *image = new unsigned short [pixels];            
		MakeNumberBitmap(i, id_s, columns, lines, image);

		// Switch the display power and backlight on.
		artista[i].setDisplayPower (true);
		artista[i].setBacklight (true);

		// Display the image.
		int err = artista[i].write (image, pixels * 2);
		delete [] image;
		artista[i].close();

		if (err == -1)
			std::cout << "ERROR: writing to screen " << i << "\n";
		else
			std::cout << "Send " << err << " bytes to '" << id_s << "'\n";
	}
}

int RubyGetIDs()
{
	// Look for for all Artista devices connected and get their number.
	std::vector<stm::Device::Descriptor> artistaDescr;
	size_t numArtistas = stm::ArtistaDevice::enumerateAll (artistaDescr, stm::ArtistaDevice::LibUsbDriverAccess);

	// Open each Artista device.
	std::vector<stm::ArtistaDevice> artista (numArtistas);
	for (size_t i = 0; i < numArtistas; ++ i)
	{
		artista [i].setDescr (artistaDescr [i]);
		artista [i].open ();
	}

	std::cout << "[";
  // For each Artista device i
  for (size_t i = 0; i < numArtistas; ++ i)
  {
		stm::Uuid id = artista[i].displayId();
		//stm::Uuid id = StringToUuid("1234567812345678");
		//std::string id_s = id.string();
		std::string id_s = UuidToString(id);	
				
		std::cout << "\"" << id_s << "\"";
		
		if (i != (numArtistas -1))
			std::cout << ", ";

		artista[i].close();
	}
	std::cout <<  "]\n";
	
	return 0; //success
}

int RubySetID(std::string old_id, std::string new_id)
{
	// Look for for all Artista devices connected and get their number.
	std::vector<stm::Device::Descriptor> artistaDescr;
	size_t numArtistas = stm::ArtistaDevice::enumerateAll (artistaDescr, stm::ArtistaDevice::LibUsbDriverAccess);

	// Open each Artista device.
	std::vector<stm::ArtistaDevice> artista (numArtistas);
	for (size_t i = 0; i < numArtistas; ++ i)
	{
		artista [i].setDescr (artistaDescr [i]);
		artista [i].open ();
	}

  // For each Artista device i
  for (size_t i = 0; i < numArtistas; ++ i)
  {
		stm::Uuid id = artista[i].displayId();
		std::string id_s = UuidToString(id);
		
		if (id_s == old_id) {
			stm::Uuid new_uuid = StringToUuid(new_id);
			artista[i].setDisplayId(new_uuid);
			artista[i].close();
			break;
		}
				
		artista[i].close();
	}
	
	return 0;
}

int RubySetIDByN(std::string n, std::string new_id)
{
	// Look for for all Artista devices connected and get their number.
	std::vector<stm::Device::Descriptor> artistaDescr;
	size_t numArtistas = stm::ArtistaDevice::enumerateAll (artistaDescr, stm::ArtistaDevice::LibUsbDriverAccess);

	//reset one screen
	int screenNum = atoi(n.c_str());
	
	if (screenNum < 0 || screenNum >= numArtistas)
	{
		std::cout << "ERROR: screen " << n << " does not exist!\n";
		exit(1);
	} 

	// Open given device
	stm::ArtistaDevice artista;
	artista.setDescr(artistaDescr[screenNum]);
	artista.open();
	artista.setDefaultTimeout(SCREEN_TIMEOUT);
	
	// Set the id
	stm::Uuid new_uuid = StringToUuid(new_id);
	artista.setDisplayId(new_uuid);		
	artista.close();

	return 0;
}

int RubyShow(std::string image_path, std::string screen_id)
{
	// Look for for all Artista devices connected and get their number.
	std::vector<stm::Device::Descriptor> artistaDescr;
	size_t numArtistas = stm::ArtistaDevice::enumerateAll (artistaDescr, stm::ArtistaDevice::LibUsbDriverAccess);

	// Open each Artista device.
	std::vector<stm::ArtistaDevice> artista (numArtistas);
	for (size_t i = 0; i < numArtistas; ++ i)
	{
		artista [i].setDescr (artistaDescr [i]);
		artista [i].open ();
	}

  // For each Artista device i
  for (size_t i = 0; i < numArtistas; ++ i)
  {
		stm::Uuid id = artista[i].displayId();
		std::string id_s = UuidToString(id);
		
		if (id_s == screen_id) {
			// show it!
			artista[i].setDefaultTimeout(SCREEN_TIMEOUT);

			// Get geometry
			int pixels = artista[i].pixels ();
			int lines = artista[i].lines ();
			int columns = artista[i].columns ();

			// Load the image
			unsigned short *image = new unsigned short [pixels];            
			ConvertToBitmap(image_path, columns, lines, image);

			// Switch the display power and backlight on.
			artista[i].setDisplayPower (true);
			artista[i].setBacklight (true);

			// Display the image.
			int err = artista[i].write (image, pixels * 2);
			artista[i].close();
			delete [] image;

			if (err == -1)
				return 1; //failure :(

			return 0; //success :)
		}
				
		artista[i].close();
	}
	
	return 1; //failure :(
}


void Help()
{
	std::cout << "artistactrl - Zoltan Olah 2007\n";
	std::cout << "Usage: artistactrl OPTION [SCREEN] [FILE]\n";
	std::cout << "Utility to control USB Artista displays\n";
	std::cout << "Options:\n";
	std::cout << "\t-l\t\tDisplay information on connected screens.\n";	 
	std::cout << "\t-s\t\tShow FILE on the given SCREEN (between 0 and number of screens)\n";	
	std::cout << "\t-r\t\tReset SCREEN (SCREEN may be 'all', then all screens are reset)\n";	
	std::cout << "\t-n\t\tNumber screens with usb ordering and id's (requires reset afterwards)\n";
	std::cout << "\t-h\t\tThis help\n";		
	std::cout << "Options designed for use with ruby libartista:\n";
	std::cout << "\t--get_ids\tReturns YAML array of screen id's\n";
	std::cout << "\t--set_id\tTakes two string parameters, sets the id for the screen\n";
	std::cout << "\t--set_id_by_n\tTakes two string parameters, sets the id for the screen based on n (the usb number)\n";
	std::cout << "\t--show\t\tTakes two string parameters, shows the file specified by first param\n";	

}

int ParseOptions(int argc, char *argv [])
{
	if (argc == 1)
	{
		Help();
	}

	std::string option = argv[1];
		
	if (option == "-l")
	{
		ArtistaList();
	} 
	else if (option == "-s")
	{
		if (argc != 4) 
		{
			std::cout << "Not enough parameters to -s option\n";
		}
		else
		{
			std::string screen = argv[2];
			std::string filename = argv[3];
			
			ArtistaShow(screen, filename);
		}
	}
	else if (option == "-r")
	{
		if (argc != 3) 
		{
			std::cout << "Not enough parameters to -r option\n";
		}
		else
		{
			std::string screen = argv[2];
			
			ArtistaReset(screen);
			
			return 0; //success
		}
	}
	else if (option == "-n")
	{
		ArtistaNumber();
	}	
	else if (option == "-h")
	{
		Help();
	}
	else if (option == "--get_ids")
	{
		return RubyGetIDs();		
	}
	else if (option == "--set_id")
	{
		std::string old_id = argv[2];
		std::string new_id = argv[3];
		
		return RubySetID(old_id, new_id);		
	}
	else if (option == "--set_id_by_n")
	{
		std::string n = argv[2];
		std::string new_id = argv[3];

		return RubySetIDByN(n, new_id);
	}	
	else if (option == "--show")
	{
		std::string image_path = argv[2];
		std::string screen_id = argv[3];
		
		return RubyShow(image_path, screen_id);		
	}		
	else
	{
		std::cout << "Illegal option: " << option << "\n";
		return 1;
	}
	
	return 1;
}

int main (int argc, char *argv [])
{
	assert (sizeof (unsigned short) == 2);
	int ret = 0;

	try
	{
		ret = ParseOptions(argc, argv);
	}
	catch (std::exception &exc)
	{
		ret = 1;
		std::string what (exc.what ());
		if (what.empty ()) what = "Unspecified error";
		else if (what [what.size () - 1] == '.') what.erase (what.size () - 1);
		std::cout << what << ": aborted.\n";
	}
	catch (...)
	{
		ret = 1;
		std::cout << " Unknown error: aborted.\n";
	}

	return ret;
}
