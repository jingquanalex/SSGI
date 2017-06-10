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
		if (shaderId)
		{
			shaderList.push_back(make_pair(shaderId, shadername));
		}
	}
	else
	{
		shaderId = it->first;
	}
}

Shader::~Shader()
{

}

void Shader::apply()
{
	glUseProgram(shaderId);
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
	const char* vertSrc = vs.c_str();
	const char* fragSrc = fs.c_str();
	const char* geomSrc = gs.c_str();

	if (vs == "") cout << "Cannot open shader: " << vspath << endl;
	if (fs == "") cout << "Cannot open shader: " << fspath << endl;
	if (vs == "" || fs == "") return 0;

	GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(vertShader, 1, &vertSrc, NULL);
	glShaderSource(fragShader, 1, &fragSrc, NULL);
	glCompileShader(vertShader);
	glCompileShader(fragShader);

	// Error checking
	int success;

	glGetShaderiv(vertShader, GL_COMPILE_STATUS, &success);
	if (success == GL_FALSE)
	{
		int logLength;
		glGetShaderiv(vertShader, GL_INFO_LOG_LENGTH, &logLength);
		vector<char> infoLog(logLength);
		glGetShaderInfoLog(vertShader, logLength, &logLength, &infoLog[0]);
		cout << "=== VERTEX SHADER ERROR ===" << endl;
		cout << &infoLog[0] << endl;
		glDeleteShader(vertShader);
		return 0;
	}

	glGetShaderiv(fragShader, GL_COMPILE_STATUS, &success);
	if (success == GL_FALSE)
	{
		int logLength;
		glGetShaderiv(fragShader, GL_INFO_LOG_LENGTH, &logLength);
		vector<char> infoLog(logLength);
		glGetShaderInfoLog(fragShader, logLength, &logLength, &infoLog[0]);
		cout << "=== FRAGMENT SHADER ERROR ===" << endl;
		cout << &infoLog[0] << endl;
		glDeleteShader(fragShader);
		return 0;
	}

	// Link shaders
	GLuint program = glCreateProgram();
	glAttachShader(program, vertShader);
	glAttachShader(program, fragShader);

	// Compile and link geometry shader if available
	GLuint geomShader;

	if (gs != "")
	{
		geomShader = glCreateShader(GL_GEOMETRY_SHADER);
		glShaderSource(geomShader, 1, &geomSrc, NULL);
		glCompileShader(geomShader);

		glGetShaderiv(geomShader, GL_COMPILE_STATUS, &success);
		if (success == GL_FALSE)
		{
			GLint logLength;
			glGetShaderiv(geomShader, GL_INFO_LOG_LENGTH, &logLength);
			vector<char> infoLog(logLength);
			glGetShaderInfoLog(geomShader, logLength, &logLength, &infoLog[0]);
			cout << "=== GEOMETRY SHADER ERROR ===" << endl;
			cout << &infoLog[0] << endl;
			glDeleteShader(geomShader);
			return 0;
		}

		glAttachShader(program, geomShader);
	}

	glLinkProgram(program);

	// Linking error checking
	glGetShaderiv(program, GL_LINK_STATUS, &success);
	if (success == GL_FALSE)
	{
		GLint logLength;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
		vector<char> infoLog(logLength);
		glGetProgramInfoLog(program, logLength, &logLength, &infoLog[0]);
		cout << "=== LINKING SHADER ERROR ===" << endl;
		cout << &infoLog[0] << endl;
		glDeleteShader(vertShader);
		glDeleteShader(fragShader);
		if (gs != "") glDeleteShader(geomShader);
		glDeleteProgram(program);
		return 0;
	}

	glDetachShader(program, vertShader);
	glDetachShader(program, fragShader);
	if (gs != "") glDetachShader(program, geomShader);

	return program;
}