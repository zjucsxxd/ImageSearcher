#include "ColorEngine.h"

ColorEngine::ColorEngine()
{
	databasePath = "color.db";
}

ColorEngine::~ColorEngine()
{
}

bool ColorEngine::start(LPCWSTR directory)
{
	// Creating variables.
	ifstream database;						// input file stream.

	// Open file.
	database.open(this->databasePath, ios::in | ios::binary);

	if (!database.is_open())
	{
		cout << "Error: Database could not be opened. Wait, creating database..." << endl;
		return createDatabase(directory);
	}

	return true;
}

color_distance* ColorEngine::searchImage(string imgPath, int quantity)
{
	// Creating variables.
	ifstream database;						// input file stream.
	HSV_data_structure imageData;			// data from requested image.
	HSV_data_structure imageFromDatabase;	// data from database image.
	color_distance *result = NULL;			// the vector of results.

	// Extract color information from original image.
	imageData = extractColor(imgPath);

	// Open file.
	database.open(this->databasePath, ios::in | ios::binary);

	if (!database.is_open())
	{
		cout << "Error: Database could not be opened." << endl;
		system("pause");
		return result;
	}

	//Initializing variables
	result = (color_distance*)malloc(sizeof(color_distance) * quantity);
	for (size_t i = 0; i < quantity; i++)
	{
		// NEEDS CORRECTION
		result[i].distance = 1 * 256 * 4 * 3;
	}

	// Generate Euclidean distance for all images within the database.
	while (!database.eof())
	{
		database.read((char *)&imageFromDatabase, sizeof(HSV_data_structure));
		float H_distance = 0;
		float S_distance = 0;
		float V_distance = 0;

		for (size_t i = 0; i < 4; i++)
		{
			// Calculating the Euclidean distances.
			for (size_t j = 0; j < 256; j++)
			{
				H_distance += euclideanDistance(imageData.H[i][j], imageFromDatabase.H[i][j]);
				S_distance += euclideanDistance(imageData.S[i][j], imageFromDatabase.S[i][j]);
				V_distance += euclideanDistance(imageData.V[i][j], imageFromDatabase.V[i][j]);
			}
		}

		// Sorting ont he fly
		color_distance aux;
		aux.distance = H_distance + S_distance + V_distance;
		aux.image = imageFromDatabase;

		for (size_t i = 0; i < quantity; i++)
		{
			if (aux.distance < result[i].distance)
			{
				for (size_t j = (quantity - 1); j > i; j--)
				{
					result[j] = result[j - 1];
				}
				result[i] = aux;
				break;
			}
		}
	}

	for (size_t i = 0; i < quantity; i++)
	{
		cout << "Image: " << result[i].image.name << " Distance: " << result[i].distance << endl;
	}

	database.close();
	return result;
}

bool ColorEngine::createDatabase(LPCWSTR directory)
{
	// Creating variables.
	ofstream ofs;								// output file stream.
	WIN32_FIND_DATA file;						// The file to be verified.
	LARGE_INTEGER filesize;						// The file size.
	TCHAR szDir[MAX_PATH];						// ?
	size_t pathLength;							// The root's path length .
	HANDLE handleFind = INVALID_HANDLE_VALUE;	// The file search handle.
	DWORD dwError = 0;							// Windows error control.
	LPCWSTR nextDirectory;						// Path to next directory in recursion.
	int counter = 0;							// Counts the number of added files.

	/// Open file
	ofs.open(this->databasePath, ios::out | ios::binary | ofstream::trunc);

	// Check that the input path.
	StringCchLength(directory, MAX_PATH, &pathLength);

	if (pathLength > (MAX_PATH - 3))
	{
		cout << "Directory path is too long." << endl;
		return false;
	}

	// Prepare string for use with FindFile functions.
	StringCchCopy(szDir, MAX_PATH, directory);
	StringCchCat(szDir, MAX_PATH, TEXT("\\*"));

	// Find the first file in the directory.
	handleFind = FindFirstFile(szDir, &file);

	// List all the files in the directory with some info about them.
	do
	{
		// convert from wide char to narrow char array
		char ch[260];
		WideCharToMultiByte(CP_ACP, 0, file.cFileName, -1, ch, 260, NULL, NULL);
		string auxName(ch);

		if (auxName.substr(auxName.find_last_of(".") + 1) == "jpg")
		{
			counter++;

			WideCharToMultiByte(CP_ACP, 0, directory, -1, ch, 260, NULL, NULL);
			string auxDirectory(ch);

			HSV_data_structure aux = extractColor(string(auxDirectory + "\\" + auxName));
			ofs.write((char*)&aux, sizeof(HSV_data_structure));
		}

	} while (FindNextFile(handleFind, &file) != 0);

	FindClose(handleFind);

	/// close file
	ofs.close();

	cout << counter << " images found." << endl;
	cout << "Database successfully created." << endl;
	return true;
}

HSV_data_structure ColorEngine::extractColor(string imgPath)
{
	// Creating variables.
	IplImage* img;					// original image.
	IplImage* imgHSV;				// HSV image.
	CvScalar pixel;					// Current pixel.
	HSV_data_structure data;		// struct where the information will be stored.

	//Initializing variables
	for (size_t i = 0; i < 4; i++)
	{
		for (size_t j = 0; j < 256; j++)
		{
			data.H[i][j] = 0.0f;
			data.S[i][j] = 0.0f;
			data.V[i][j] = 0.0f;
		}
	}

	for (size_t i = 0; i < 4; i++)
	{
		data.pixelCounter[i] = 0;
	}

	string aux = imgPath.substr(imgPath.find_last_of("\\") + 1);
	for (size_t i = 0; i < MAX_PATH; i++) data.name[i] = '\0';
	for (size_t i = 0; i < aux.length(); i++) data.name[i] = aux[i];

	// Loading image
	if ((img = cvLoadImage(imgPath.c_str(), CV_LOAD_IMAGE_UNCHANGED)) == NULL)
	{
		cout << endl << "Erro: Imagem nao pode ser carregada." << endl << endl;
		system("pause");
		return data;
	}

	// Converting image to HSV
	imgHSV = cvCreateImage(cvSize(img->width, img->height), IPL_DEPTH_8U, 3);

	cvCvtColor(img, imgHSV, CV_BGR2HSV);

	if (imgHSV == NULL)
	{
		cout << "Erro: Imagem nao pode ser transformada." << endl << endl;
		system("pause");
		return data;
	}

	// Creating histogram
	int halfHeight = imgHSV->height / 2;
	int halfWidth = imgHSV->width / 2;
	int quadrant;

	for (size_t height = 0; height < imgHSV->height; height++)
	{
		for (size_t width = 0; width < imgHSV->width; width++)
		{
			pixel = cvGet2D(imgHSV, height, width);

			if (height < halfHeight && width < halfWidth) { quadrant = 0; }
			else if (height < halfHeight && width >= halfWidth) { quadrant = 1; }
			else if (height >= halfHeight && width < halfWidth) { quadrant = 2; }
			else if (height >= halfHeight && width >= halfWidth) { quadrant = 3; }
			else { cout << "Error: Invalide pixel coordinate." << endl; }

			data.H[quadrant][(int)pixel.val[0]]++;
			data.S[quadrant][(int)pixel.val[1]]++;
			data.V[quadrant][(int)pixel.val[2]]++;
			data.pixelCounter[quadrant]++;
		}
	}

	// Normalizing HSV data
	for (size_t i = 0; i < SECTIONS; i++)
	{
		for (size_t j = 0; j < LEVELS; j++)
		{
			data.H[i][j] /= data.pixelCounter[i];
			data.S[i][j] /= data.pixelCounter[i];
			data.V[i][j] /= data.pixelCounter[i];
		}
	}

	cvReleaseImage(&img);
	cvReleaseImage(&imgHSV);
	return data;
}

float ColorEngine::euclideanDistance(float value1, float value2)
{
	double value = value1 - value2;
	double distance;

	distance = pow(value, 2);           // calculating distance by euclidean formula
	distance = sqrt(distance);          // sqrt is function in math.h

	return distance;
}