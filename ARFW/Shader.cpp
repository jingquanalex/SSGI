#include "Shader.h"

using namespace std;
using namespace glm;

extern std::string g_ExePath;
vector<Shader::IdNamePair> Shader::shaderList;

Shader::Shader(string shadername)
{
	if (shadername == "") return;

	// Check program list for the shader and set id, if unavailable, read file and compile
	vector<IdNamePair>::iterator it = find_if(shaderList.begin(), shaderList.end(),
		[shadername](const IdNamePair& e){ return e.second == shadername; });

	if (it == shaderList.end())
	{
		cout << "Shader Loaded: " << shadername << endl;
		shaderId = compileShader(shadername);
		shaderName = shadername;
		if (shaderId)
		{
			shaderList.push_back(make_pair(shaderId, shadername));
		}
	}
	else
	{
		shaderId = it->first;
		shaderName = it->second;
	}
}

Shader::~Shader()
{

}

void Shader::apply()
{
	glUseProgram(shaderId);
}

void Shader::recompile()
{
	cout << "Shader recompiled: " << shaderName << endl;
	if (shaderId != 0) compileShader(shaderName);
}

GLuint Shader::getShaderId() const
{
	return shaderId;
}

std::string Shader::getShaderName() const
{
	return shaderName;
}

// Read and return the contents of a file.
string Shader::readShaderFile(string filename)
{
	ifstream fileStream(filename, ios::in);
	if (fileStream.is_open())
	{
		stringstream strStream;
		strStream << fileStream.rdbuf();
		fileStream.close();

		return strStream.str();
	}

	return "";
}

// Load, compile and link shader and return program id.
GLuint Shader::compileShader(string name)
{
	// Read and compile shader files
	string vspath = g_ExePath + "../../media/shader/" + name + ".vs";
	string fspath = g_ExePath + "../../media/shader/" + name + ".fs";
	string gspath = g_ExePath + "../../media/shader/" + name + ".gs";
	string vs = readShaderFile(vspath);
	string fs = readShaderFile(fspath);
	string gs = readShaderFile(gspath);
	const GLchar* vertSrc = vs.c_str();
	const GLchar* fragSrc = fs.c_str();
	const GLchar* geomSrc = gs.c_str();

	if (vs == "") cout << "Cannot open shader: " << vspath << endl;
	if (fs == "") cout << "Cannot open shader: " << fspath << endl;
	if (vs == "" || fs == "") return 0;

	GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(vertShader, 1, &vertSrc, NULL);
	glShaderSource(fragShader, 1, &fragSrc, NULL);
	glCompileShader(vertShader);
	glCompileShader(fragShader);

	glGetShaderiv(vertShader, GL_COMPILE_STATUS, &status);
	glGetShaderiv(vertShader, GL_INFO_LOG_LENGTH, &logLength);
	infoLog = vector<GLchar>(logLength);
	glGetShaderInfoLog(vertShader, logLength, &logLength, &infoLog[0]);
	if (logLength > 0)
	{
		cout << "=== VERTEX SHADER LOG ===" << endl;
		cout << &infoLog[0] << endl;
	}

	if (status == GL_FALSE)
	{
		glDeleteShader(vertShader);
		return 0;
	}

	glGetShaderiv(fragShader, GL_COMPILE_STATUS, &status);
	glGetShaderiv(fragShader, GL_INFO_LOG_LENGTH, &logLength);
	infoLog = vector<GLchar>(logLength);
	glGetShaderInfoLog(fragShader, logLength, &logLength, &infoLog[0]);
	if (logLength > 0)
	{
		cout << "=== FRAGMENT SHADER LOG ===" << endl;
		cout << &infoLog[0] << endl;
	}

	if (status == GL_FALSE)
	{
		glDeleteShader(fragShader);
		return 0;
	}

	// Link shaders
	if (shaderId == 0) shaderId = glCreateProgram();
	glAttachShader(shaderId, vertShader);
	glAttachShader(shaderId, fragShader);

	// Compile and link geometry shader if available
	GLuint geomShader;

	if (gs != "")
	{
		geomShader = glCreateShader(GL_GEOMETRY_SHADER);
		glShaderSource(geomShader, 1, &geomSrc, NULL);
		glCompileShader(geomShader);

		glGetShaderiv(geomShader, GL_COMPILE_STATUS, &status);
		glGetShaderiv(geomShader, GL_INFO_LOG_LENGTH, &logLength);
		infoLog = vector<GLchar>(logLength);
		glGetShaderInfoLog(geomShader, logLength, &logLength, &infoLog[0]);
		if (logLength > 0)
		{
			cout << "=== GEOMETRY SHADER LOG ===" << endl;
			cout << &infoLog[0] << endl;
		}

		if (status == GL_FALSE)
		{
			glDeleteShader(geomShader);
			return 0;
		}

		glAttachShader(shaderId, geomShader);
	}

	glLinkProgram(shaderId);

	// Linking error checking
	glGetShaderiv(shaderId, GL_LINK_STATUS, &status);
	glGetProgramiv(shaderId, GL_INFO_LOG_LENGTH, &logLength);
	infoLog = vector<GLchar>(logLength);
	glGetProgramInfoLog(shaderId, logLength, &logLength, &infoLog[0]);
	if (logLength > 0)
	{
		cout << "=== PROGRAM LINKING LOG ===" << endl;
		cout << &infoLog[0] << endl;
	}
	
	if (status == GL_FALSE)
	{
		glDeleteShader(vertShader);
		glDeleteShader(fragShader);
		if (gs != "") glDeleteShader(geomShader);
		glDeleteProgram(shaderId);
		return 0;
	}

	glDetachShader(shaderId, vertShader);
	glDetachShader(shaderId, fragShader);
	if (gs != "") glDetachShader(shaderId, geomShader);

	return shaderId;
}