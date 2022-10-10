
#include <iostream>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <filesystem>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"


#include <fmt/os.h>


struct Vertex
{
    Vertex(){}
    Vertex(const glm::vec3& p,const glm::vec3& n,const glm::vec3& c,const glm::vec2& tex)
        :position(p),normal(n),color(c),uv(tex){}

    Vertex(
        float px, float py, float pz,
        float nx, float ny, float nz,
        float cx, float cy, float cz,
        float u, float v)
        : position(px, py, pz), normal(nx, ny, nz),
        color(cx, cy, cz), uv(u, v) {}

    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 color;
    glm::vec2 uv;
};

struct MeshData
{
    std::string name;
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
};

namespace fs = std::filesystem;
struct Texture
{
	Texture(const std::string& path){
	
		if(fs::exists(path))
		{
		    data = stbi_load(path.c_str(), &x, &y, &z, 0);
		}
	};

	glm::vec3 getColorNear(float uvx,float uvy)
	{
		float posx = uvx * (x - 1);
		float posy = uvy * (y - 1);

		auto neg_x = (int)(posx);
		auto neg_y = (int)(posy);

		auto idx = (neg_y * x + neg_x) * z;

		return glm::vec3(data[idx],data[idx+1],data[idx+2]);
	};

	glm::vec3 getColorAt(int idx,int idy)
	{
		auto id = (idy * x + idx) * z;

		return glm::vec3(data[id], data[id + 1], data[id + 2]);
	}
	/// <summary>
	/// 
	/// </summary>
	/// <param name="uvx">0~1</param>
	/// <param name="uvy">0~1</param>
	/// <returns></returns>
	glm::vec3 getColor(float uvx,float uvy){
		float posx = uvx * (x-1);
		float posy = uvy * (y-1);

		auto neg_x = (int)(posx);
		auto neg_y = (int)(posy);

		auto pos_x = (int)(uvx * x)+1;
		auto pos_y = (int)(uvy * y)+1;

		neg_x = glm::clamp(neg_x, 0, x-1);
		neg_y = glm::clamp(neg_y, 0, y-1);
		pos_x = glm::clamp(pos_x, 0, x-1);
		pos_y = glm::clamp(pos_y, 0, y-1);

		float off_x = posx - neg_x;
		float off_y = posy - neg_y;

		//
		//   0---------------------1
		//
		//
		//
		//   3                     2
		glm::vec3 color_0 = getColorAt(neg_x, neg_y);
		glm::vec3 color_1 = getColorAt(pos_x, neg_y);
		glm::vec3 color_2 = getColorAt(pos_x, pos_y);
		glm::vec3 color_3 = getColorAt(neg_x, pos_y);

		auto lerpfunc = [](const glm::vec3& a,const glm::vec3& b,float ratio){ return a*(1.0f - ratio) + ratio * b;};
		auto color01 = lerpfunc(color_0,color_1,off_x);
		auto color32 = lerpfunc(color_3,color_2,off_x);
		auto color = lerpfunc(color01,color32,off_y);// / 255.0f;

		return color;
	};

	~Texture()
	{
		delete []data;
	}
	unsigned char* data;
	int x,y,z;
};
bool loadFromFile(const std::string& file_path,MeshData& mesh_data)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string warn;
	std::string err;
	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, file_path.c_str());

	//读取报错
	if (!err.empty()) std::cout<<err;
	//读取失败
	if(!ret) return false;

	//初始化
	mesh_data.vertices.clear();
	mesh_data.indices.clear();

	bool recalculate_normal = false;
	// Loop over shapes
	for (size_t s = 0; s < shapes.size(); s++) {
		// Loop over faces(polygon)
		size_t index_offset = 0;
		for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
			size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);

			// Loop over vertices in the face.
			for (size_t v = 0; v < fv; v++) {
				Vertex vertex;
				// access to vertex
				tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

				tinyobj::real_t vx = attrib.vertices[3 * size_t(idx.vertex_index) + 0];
				tinyobj::real_t vy = attrib.vertices[3 * size_t(idx.vertex_index) + 1];
				tinyobj::real_t vz = attrib.vertices[3 * size_t(idx.vertex_index) + 2];

				vertex.position = glm::vec3(vx,vy,vz);
				// Check if `normal_index` is zero or positive. negative = no normal data
				if (idx.normal_index >= 0) {
					tinyobj::real_t nx = attrib.normals[3 * size_t(idx.normal_index) + 0];
					tinyobj::real_t ny = attrib.normals[3 * size_t(idx.normal_index) + 1];
					tinyobj::real_t nz = attrib.normals[3 * size_t(idx.normal_index) + 2];

					vertex.normal = glm::normalize(glm::vec3(nx,ny,nz));
				}
				else
				{
					vertex.normal = glm::vec3(0,1,0);
					recalculate_normal = true;
				}

				// Check if `texcoord_index` is zero or positive. negative = no texcoord data
				if (idx.texcoord_index >= 0) {
					tinyobj::real_t tx = attrib.texcoords[2 * size_t(idx.texcoord_index) + 0];
					tinyobj::real_t ty = attrib.texcoords[2 * size_t(idx.texcoord_index) + 1];

					vertex.uv = glm::vec2(tx,ty);
				}
				else
				{
					vertex.uv = glm::vec2(0,0);
				}
				// Optional: vertex colors
				// tinyobj::real_t red   = attrib.colors[3*size_t(idx.vertex_index)+0];
				// tinyobj::real_t green = attrib.colors[3*size_t(idx.vertex_index)+1];
				// tinyobj::real_t blue  = attrib.colors[3*size_t(idx.vertex_index)+2];

				vertex.color = glm::vec3(0);
				mesh_data.vertices.emplace_back(vertex);

			}
			index_offset += fv;

			// per-face material
			shapes[s].mesh.material_ids[f];
		}

		for(unsigned int i=0;i<mesh_data.vertices.size();i++)
		{
			mesh_data.indices.emplace_back(i);
		}
	}

	return true;
};

void fetchColorFromTex(MeshData& mesh,Texture& tex)
{
	for(auto& v: mesh.vertices)
	{
		auto color = tex.getColor(v.uv.x,1.0-v.uv.y);
		v.color = color;
	}
}

void writeToPly(MeshData& mesh)
{
	auto out = fmt::output_file("model_debug.ply");
	out.print("{0}\n","ply");//
	out.print("{0}\n","format ascii 1.0");//
	out.print("{0}\n","comment author: Greg Turk");//
	out.print("{0}\n","comment object: another cube");//
	out.print("{0} {1}\n","element vertex ",mesh.vertices.size());//
	out.print("{0}\n","property float x");//
	out.print("{0}\n","property float y");//
	out.print("{0}\n","property float z");//
	out.print("{0}\n","property uchar red");//                    { start of vertex color, vertex 元素 颜色属性 }
	out.print("{0}\n","property uchar green");//
	out.print("{0}\n","property uchar blue");//
	out.print("{0} {1}\n","element face ",mesh.indices.size()/3);//
	out.print("{0}\n","property list uchar int vertex_index");//  { (uchar) number of vertices for each face (每一个面的顶点个数)，(int) 顶点的索引  }
	out.print("{0}\n","end_header");//

	//vertex
	for(auto v : mesh.vertices)
	{
		out.print("{0} {1} {2} {3} {4} {5}\n",v.position.x,v.position.y,v.position.z,
			(int)v.color.x,
			(int)v.color.y,
			(int)v.color.z);
	}

	//face
	for(int i=0;i<mesh.indices.size();i+=3)
	{
		out.print("3 {0} {1} {2}\n",
			mesh.indices[i],
			mesh.indices[i+1],
			mesh.indices[i+2]);
	}
	out.flush();
	out.close();
}

int main()
{
	MeshData mesh;
	loadFromFile(R"(D:\projects\vtkShowObjTexture\model.obj)",mesh);

	Texture tex(R"(D:\projects\vtkShowObjTexture\model.png)");

	fetchColorFromTex(mesh,tex);

	writeToPly(mesh);


    return 0;
}