#include "Object3D.h"
#include "ply.h"

Object3D::Object3D()
{
	vlist = nullptr;
	tlist = nullptr;
}


Object3D::~Object3D()
{
	if (vlist != nullptr)
	{
		delete[] vlist;
		vlist = nullptr;
	}
	if (tlist != nullptr)
	{
		delete[] tlist;
		tlist = nullptr;
	}
}

void Object3D::CreateObject(const std::string& ply_file_path)
{
	PlyFile* ply;
	int nelems;
	char **elist;
	int file_type;
	float version;

	//List of vertex propreties
	//Define this in the order you define attributes in the .ply file. 
	//Additionally be careful that your definiton of Vertex in the GeometricDefinitions.h corresponds
	//to the vertex definition in the .ply file.
	// First element is the attribute name
	// Second and third elements are the attribute type (float)
	// Fourth element is the offset of the attribute in the structure. offsetof(.,.) can be used for this 
	//purpose
	// Fifth element shows whether this attribute is a scalar (0) or a list (1)
	// For list attributes, an implicit variable is needed, 
	// which shows the count of the elements in the list.
	// Sixth and seventh elements are type of this count variable, if it exists.
	// Eight element is the offset of this count variable in the structure.

	PlyProperty vert_props[] = 
	{ 
		{ "x", PLY_FLOAT, PLY_FLOAT, offsetof(Vertex,x), 0, 0, 0, 0 },
		{ "y", PLY_FLOAT, PLY_FLOAT, offsetof(Vertex,y), 0, 0, 0, 0 },
		{ "z", PLY_FLOAT, PLY_FLOAT, offsetof(Vertex,z), 0, 0, 0, 0 },
		{ "r", PLY_FLOAT, PLY_FLOAT, offsetof(Vertex,r), 0, 0, 0, 0 },
		{ "g", PLY_FLOAT, PLY_FLOAT, offsetof(Vertex,g), 0, 0, 0, 0 },
		{ "b", PLY_FLOAT, PLY_FLOAT, offsetof(Vertex,b), 0, 0, 0, 0 }
	};
	int attributeCount = sizeof(vert_props) / sizeof(PlyProperty);

	//List of triangle properties. This is a index list, so we need to specify information
	//about the count variable as well.
	PlyProperty tri_props[] =
	{
		{ "vertex_indices", PLY_INT, PLY_INT, offsetof(Triangle,verts),
		1, PLY_UCHAR, PLY_UCHAR, offsetof(Triangle,nverts) },
	};

	char* c_file = (char*)ply_file_path.c_str();
	//Read the .ply file. We will have "vertex" and "triangle" elements in the .ply file.
	// The following descriptions are based on the basic .ply I am providing.
	// nelems will hold the number of the elements (should be 2).
	// elist will hold the names of the elements. (should be "vertex" and "triangle").
	ply = ply_open_for_reading(c_file, &nelems, &elist, &file_type, &version);

	// Go through each kind of element that we learned is in the file and read them */
	for (int i = 0; i < nelems; i++) 
	{
		std::string element_name(elist[i]);
		int num_elems;
		int nprops;
		// num_elems will hold the number of distinct copies for the current element.
		// For example for the rectangle.ply, for the "vertex" element, we have 4 distinct vertices.
		// nprops holds the number of properties (attributes). 
		// In rectangle.ply, we have 6 properties, 3 for the position, 3 for the color
		PlyProperty **plist = ply_get_element_description(ply, elist[i], &num_elems, &nprops);
		//If "vertex"
		if (element_name == "vertex")
		{
			vertexCount = num_elems;
			vlist = new Vertex[vertexCount];
			//Set the properties, inform the .ply reader about our attribute. We have set
			//that in vert_props array.
			for (int j = 0; j < nprops; j++)
				ply_get_property(ply, elist[i], &vert_props[j]);
			//Read in the vertex values
			for (int j = 0; j < vertexCount; j++)
				ply_get_element(ply, &vlist[j]);
		}
		//If "triangle"
		else if (element_name == "triangle")
		{
			triangleCount = num_elems;
			tlist = new Triangle[triangleCount];
			for (int j = 0; j < nprops; j++)
				ply_get_property(ply, elist[i], &tri_props[j]);
			//Read in the vertex values
			for (int j = 0; j < triangleCount; j++)
				ply_get_element(ply, &tlist[j]);
		}
	}
	ply_close(ply);

	
	//Bind the vertex and index buffers
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);
	// Bind our Vertex Array Object first, then bind and set our buffers and pointers.
	glBindVertexArray(VAO);
	//Convert our vertex list into a continuous array, copy the vertices into the vertex buffer.
	float* vertexData = new float[vertexCount * attributeCount];
	for (int i = 0; i < vertexCount; i++)
		memcpy(&vertexData[i*attributeCount], 
			vlist[i].getAsArray(), sizeof(float)*attributeCount);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*attributeCount*vertexCount, vertexData, GL_STATIC_DRAW);
	//Copy the index data found in the list of triangles into the element array buffer (index array)
	//We are using a triangles, so we need triangleCount * 3 indices.
	int* indexData = new int[triangleCount * 3];
	for (int i = 0; i < triangleCount; i++)
		memcpy(&indexData[i * 3], tlist[i].verts, sizeof(int) * 3);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int)*3*triangleCount, indexData, GL_STATIC_DRAW);
	// Position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, attributeCount * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);
	// Color attribute
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, attributeCount * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);
	// Unbind VAO
	glBindVertexArray(0);

	// Delete temporary buffers
	delete[] vertexData;
	delete[] indexData;
}