#pragma once


struct Vertex {
	float x, y, z;             /* the usual 3-space position of a vertex */
	float r, g, b;             // Color

	//Don't forget to modify this method if you are to change the vertex structure.þ
	float* getAsArray()
	{
		//Keep the declaration order
		float attributeArray[] = { x,y,z,r,g,b};
		return attributeArray;
	}
};

struct Triangle {
	unsigned char nverts;    /* number of vertex indices in list */
	int *verts;              /* vertex index list */
};
