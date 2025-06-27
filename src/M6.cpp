#include <iostream>
#include <string>
#include <assert.h>
#include <filesystem>
#include <fstream>
#include <vector>
#include <sstream>
#include <map>
#include <cmath>

using namespace std;

// Definir M_PI se não estiver definido
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

//GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// Protótipo da função de callback de teclado
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);

// Protótipos das funções
int setupShader();
int setupGeometry();

// Protótipos para controle de câmera
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void processInput(GLFWwindow* window);

// Estrutura para geometria carregada de arquivo OBJ
struct Geometry
{
	GLuint VAO;
	GLuint vertexCount;
	GLuint textureID = 0;
	string textureFilePath;
};

// Estrutura para representar um ponto de controle da trajetória
struct ControlPoint
{
	glm::vec3 position;
	float time; // Tempo para chegar neste ponto (em segundos)
	
	ControlPoint(glm::vec3 pos = glm::vec3(0.0f), float t = 1.0f) 
		: position(pos), time(t) {}
};

// Classe para gerenciar trajetórias
class Trajectory
{
private:
	vector<ControlPoint> controlPoints;
	float totalTime;
	float currentTime;
	int currentSegment;
	bool isActive;
	
public:
	Trajectory() : totalTime(0.0f), currentTime(0.0f), currentSegment(0), isActive(false) {}
	
	void addControlPoint(const glm::vec3& position, float time = 1.0f)
	{
		controlPoints.push_back(ControlPoint(position, time));
		updateTotalTime();
	}
	
	void clearControlPoints()
	{
		controlPoints.clear();
		totalTime = 0.0f;
		currentTime = 0.0f;
		currentSegment = 0;
		isActive = false;
	}
	
	void start()
	{
		if (controlPoints.size() >= 2)
		{
			isActive = true;
			currentTime = 0.0f;
			currentSegment = 0;
		}
	}
	
	void stop()
	{
		isActive = false;
	}
	
	void update(float deltaTime)
	{
		if (!isActive || controlPoints.size() < 2)
			return;
			
		currentTime += deltaTime;
		
		// Verifica se chegou ao final do segmento atual
		if (currentTime >= controlPoints[currentSegment].time)
		{
			currentTime = 0.0f;
			currentSegment++;
			
			// Se chegou ao final da trajetória, volta ao início (cíclica)
			if (currentSegment >= controlPoints.size())
			{
				currentSegment = 0;
			}
		}
	}
	
	glm::vec3 getCurrentPosition()
	{
		if (controlPoints.size() < 2)
			return glm::vec3(0.0f);
			
		if (controlPoints.size() == 1)
			return controlPoints[0].position;
			
		// Interpolação linear entre pontos
		int nextSegment = (currentSegment + 1) % controlPoints.size();
		float t = currentTime / controlPoints[currentSegment].time;
		
		return glm::mix(controlPoints[currentSegment].position, 
					   controlPoints[nextSegment].position, t);
	}
	
	bool isRunning() const { return isActive; }
	size_t getPointCount() const { return controlPoints.size(); }
	
	// Método para acessar um ponto de controle específico
	const ControlPoint& getControlPoint(size_t index) const 
	{ 
		if (index < controlPoints.size())
			return controlPoints[index];
		static ControlPoint defaultPoint;
		return defaultPoint;
	}
	
	// Método para obter todos os pontos de controle
	const vector<ControlPoint>& getControlPoints() const { return controlPoints; }
	
	// Salvar trajetória em arquivo
	bool saveToFile(const string& filename)
	{
		ofstream file(filename);
		if (!file.is_open())
			return false;
			
		file << controlPoints.size() << endl;
		for (const auto& point : controlPoints)
		{
			file << point.position.x << " " << point.position.y << " " << point.position.z 
				 << " " << point.time << endl;
		}
		file.close();
		return true;
	}
	
	// Carregar trajetória de arquivo
	bool loadFromFile(const string& filename)
	{
		ifstream file(filename);
		if (!file.is_open())
			return false;
			
		clearControlPoints();
		
		int pointCount;
		file >> pointCount;
		
		for (int i = 0; i < pointCount; ++i)
		{
			glm::vec3 pos;
			float time;
			file >> pos.x >> pos.y >> pos.z >> time;
			addControlPoint(pos, time);
		}
		
		file.close();
		return true;
	}
	
private:
	void updateTotalTime()
	{
		totalTime = 0.0f;
		for (const auto& point : controlPoints)
		{
			totalTime += point.time;
		}
	}
};

// Estrutura da câmera em primeira pessoa
struct FirstPersonCamera
{
	glm::vec3 position;
	glm::vec3 front;
	glm::vec3 up;
	glm::vec3 right;
	glm::vec3 worldUp;
	
	float yaw;
	float pitch;
	float movementSpeed;
	float mouseSensitivity;
	
	FirstPersonCamera(glm::vec3 startPosition = glm::vec3(0.0f, 0.0f, 3.0f))
		: position(startPosition), worldUp(glm::vec3(0.0f, 1.0f, 0.0f)), 
		  yaw(-90.0f), pitch(0.0f), movementSpeed(2.5f), mouseSensitivity(0.1f)
	{
		updateCameraVectors();
	}
	
	void updateCameraVectors()
	{
		glm::vec3 direction;
		direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
		direction.y = sin(glm::radians(pitch));
		direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
		
		front = glm::normalize(direction);
		right = glm::normalize(glm::cross(front, worldUp));
		up = glm::normalize(glm::cross(right, front));
	}
	
	glm::mat4 getViewMatrix()
	{
		return glm::lookAt(position, position + front, up);
	}
	
	void processKeyboard(const std::string& direction, float deltaTime)
	{
		float velocity = movementSpeed * deltaTime;
		
		if (direction == "FORWARD")
			position += front * velocity;
		if (direction == "BACKWARD")
			position -= front * velocity;
		if (direction == "LEFT")
			position -= right * velocity;
		if (direction == "RIGHT")
			position += right * velocity;
		if (direction == "UP")
			position += worldUp * velocity;
		if (direction == "DOWN")
			position -= worldUp * velocity;
	}
	
	void processMouseMovement(float xoffset, float yoffset, bool constrainPitch = true)
	{
		xoffset *= mouseSensitivity;
		yoffset *= mouseSensitivity;
		
		yaw += xoffset;
		pitch += yoffset;
		
		if (constrainPitch)
		{
			if (pitch > 89.0f)
				pitch = 89.0f;
			if (pitch < -89.0f)
				pitch = -89.0f;
		}
		
		updateCameraVectors();
	}
};

// Estrutura para objetos da cena
struct SceneObject
{
	Geometry geometry;
	Trajectory trajectory;
	glm::vec3 position;
	glm::vec3 rotation;
	glm::vec3 scale;
	string name;
	
	SceneObject(const string& objName = "Object") 
		: position(0.0f), rotation(0.0f), scale(1.0f), name(objName) {}
};

// Funções para carregamento de objeto OBJ
Geometry setupGeometryFromFile(const char* filepath);
bool loadObject(
    const char* path,
    std::vector<glm::vec3>& out_vertices,
    std::vector<glm::vec2>& out_uvs,
    std::vector<glm::vec3>& out_normals);
int loadTexture(const string& path);
string loadMTL(const string& path);

// Função para renderizar pontos de controle da trajetória
void renderTrajectoryPoints(const Trajectory& trajectory, GLuint shaderID, 
                           const glm::mat4& view, const glm::mat4& projection);

// Função para criar geometria de pontos de controle
GLuint createControlPointGeometry();

// Dimensões da janela (pode ser alterado em tempo de execução)
const GLuint WIDTH = 1000, HEIGHT = 1000;

// Código fonte do Vertex Shader (em GLSL): ainda hardcoded
const GLchar* vertexShaderSource = "#version 450\n"
"layout (location = 0) in vec3 position;\n"
"layout (location = 1) in vec3 color;\n"
"layout (location = 2) in vec2 tex_coord;\n"
"layout (location = 3) in vec3 normal;\n"
"\n"
"uniform mat4 model;\n"
"uniform mat4 view;\n"
"uniform mat4 projection;\n"
"\n"
"out vec4 finalColor;\n"
"out vec2 texCoord;\n"
"out vec3 fragPos;\n"
"out vec3 fragNormal;\n"
"\n"
"void main()\n"
"{\n"
"    vec4 worldPos = model * vec4(position, 1.0);\n"
"    gl_Position = projection * view * worldPos;\n"
"    finalColor = vec4(color, 1.0);\n"
"    texCoord = vec2(tex_coord.x, 1 - tex_coord.y);\n"
"    fragPos = vec3(worldPos);\n"
"    fragNormal = mat3(transpose(inverse(model))) * normal;\n"
"}\0";

//Códifo fonte do Fragment Shader (em GLSL): ainda hardcoded
const GLchar* fragmentShaderSource = "#version 450\n"
"in vec4 finalColor;\n"
"in vec2 texCoord;\n"
"in vec3 fragPos;\n"
"in vec3 fragNormal;\n"
"out vec4 color;\n"
"uniform sampler2D tex_buffer;\n"
"uniform vec3 ka;\n"
"uniform vec3 kd;\n"
"uniform vec3 ks;\n"
"uniform float q;\n"
"uniform vec3 lightPos;\n"
"uniform vec3 lightColor;\n"
"uniform vec3 cameraPos;\n"
"void main()\n"
"{\n"
"    vec3 ambient = lightColor * ka;\n"
"    vec3 N = normalize(fragNormal);\n"
"    vec3 L = normalize(lightPos - fragPos);\n"
"    float diff = max(dot(N, L), 0.0);\n"
"    vec3 diffuse = diff * lightColor * kd;\n"
"    vec3 R = reflect(-L, N);\n"
"    vec3 V = normalize(cameraPos - fragPos);\n"
"    float spec = pow(max(dot(R, V), 0.0), q);\n"
"    vec3 specular = spec * ks * lightColor;\n"
"    vec3 texColor = texture(tex_buffer, texCoord).rgb;\n"
"    vec3 result = (ambient + diffuse) * texColor + specular;\n"
"    color = vec4(result, 1.0f);\n"
"}\n\0";

bool rotateX=false, rotateY=false, rotateZ=false;

string mtlFilePath = "";

// Variáveis para iluminação de Phong
glm::vec3 ambientColor(0.2f, 0.2f, 0.2f), diffuseColor(0.7f, 0.7f, 0.7f), specularColor(0.5f, 0.5f, 0.5f), emissiveColor(0.0f);
float     diffuseScalar = 0.8f;
float     shininess     = 16.0f;

// Variáveis para controle de câmera em primeira pessoa
FirstPersonCamera camera(glm::vec3(0.0f, 0.0f, 5.0f));
bool firstMouse = true;
float lastX = WIDTH / 2.0f;
float lastY = HEIGHT / 2.0f;
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Variáveis para sistema de trajetórias
vector<SceneObject> sceneObjects;
int selectedObjectIndex = 0;
bool trajectoryMode = false;
bool showTrajectoryPoints = false;

// Função para mostrar instruções de uso
void showInstructions()
{
    cout << "=== SISTEMA DE TRAJETÓRIAS ===" << endl;
    cout << "T - Ativar/Desativar modo trajetória" << endl;
    cout << "P - Adicionar ponto de controle (no modo trajetória)" << endl;
    cout << "Clique Esquerdo - Adicionar ponto de controle (no modo trajetória)" << endl;
    cout << "SPACE - Iniciar/Parar trajetória" << endl;
    cout << "C - Limpar trajetória" << endl;
    cout << "S - Salvar trajetória em arquivo" << endl;
    cout << "L - Carregar trajetória de arquivo" << endl;
    cout << "O - Selecionar próximo objeto" << endl;
    cout << "V - Mostrar/Esconder pontos de controle" << endl;
    cout << "I - Mostrar informações da trajetória" << endl;
    cout << "1 - Criar trajetória circular" << endl;
    cout << "2 - Criar trajetória quadrada" << endl;
    cout << "3 - Criar trajetória triangular" << endl;
    cout << "X/Y/Z - Rotação do objeto" << endl;
    cout << "WASD - Movimento da câmera" << endl;
    cout << "Mouse - Olhar ao redor" << endl;
    cout << "ESC - Sair" << endl;
    cout << "=============================" << endl;
}

// Função MAIN
int main()
{
    // Inicialização da GLFW
    glfwInit();

    // Versão do OpenGL
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Criação da janela
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Ola 3D -- Eduardo!", nullptr, nullptr);
    glfwMakeContextCurrent(window);

    // Callback de teclado
    glfwSetKeyCallback(window, key_callback);

    // Callbacks para câmera em primeira pessoa
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Inicializa GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Informações da GPU
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "OpenGL version supported: " << glGetString(GL_VERSION) << std::endl;

    // Mostrar instruções de uso
    showInstructions();

    // Configuração da viewport
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    // Compilação dos shaders e geometria
    GLuint shaderID = setupShader();
    
    // Carregamento do objeto Suzanne
    Geometry suzanneGeometry = setupGeometryFromFile("C:/Users/educo/Desktop/CG-CC-01-25/assets/Modelos3D/SuzanneSubdiv1.obj");

    // Criar objetos da cena
    SceneObject suzanne("Suzanne");
    suzanne.geometry = suzanneGeometry;
    suzanne.position = glm::vec3(0.0f, 0.0f, 0.0f);
    sceneObjects.push_back(suzanne);

    glUseProgram(shaderID);

    // Localizações dos uniforms
    GLint modelLoc = glGetUniformLocation(shaderID, "model");
    GLint viewLoc = glGetUniformLocation(shaderID, "view");
    GLint projLoc = glGetUniformLocation(shaderID, "projection");

    glEnable(GL_DEPTH_TEST);

    // Loop da aplicação - "game loop"
    while (!glfwWindowShouldClose(window))
    {
        // Calcula delta time
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Checa se houveram eventos de input (key pressed, mouse moved etc.) e chama as funções de callback correspondentes
		glfwPollEvents();

        // Processa input contínuo
        processInput(window);

        // Atualiza trajetórias
        for (auto& obj : sceneObjects)
        {
            if (obj.trajectory.isRunning())
            {
                obj.trajectory.update(deltaTime);
                obj.position = obj.trajectory.getCurrentPosition();
            }
        }

        // Limpa buffer de cor
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float angle = (GLfloat)glfwGetTime();

        // MATRIZ DA CAMERA usando a nova câmera em primeira pessoa
        glm::mat4 view = camera.getViewMatrix();

        // MATRIZ PARA ZOOM
        glm::mat4 projection = glm::perspective(
            glm::radians(45.0f),
            (float)width / height,
            0.1f,
            100.0f
        );

        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        glLineWidth(10);
        glPointSize(20);
        
        // Configuração da iluminação de Phong
        glUniform3f(glGetUniformLocation(shaderID, "ka"), ambientColor.r, ambientColor.g, ambientColor.b);
        glUniform3f(glGetUniformLocation(shaderID, "kd"), diffuseColor.r, diffuseColor.g, diffuseColor.b);
        glUniform3f(glGetUniformLocation(shaderID, "ks"), specularColor.r, specularColor.g, specularColor.b);
        glUniform1f(glGetUniformLocation(shaderID, "q"), shininess);

        // Luz posicionada na frente do personagem (Suzanne)
        glUniform3f(glGetUniformLocation(shaderID, "lightPos"), 0.0f, 0.0f, 5.0f);
        // Iluminação mais suave
        glUniform3f(glGetUniformLocation(shaderID, "lightColor"), 0.8f, 0.8f, 0.8f);
        glUniform3f(glGetUniformLocation(shaderID, "cameraPos"), camera.position.x, camera.position.y, camera.position.z);
        
        // Renderização dos objetos da cena
        for (size_t i = 0; i < sceneObjects.size(); ++i)
        {
            auto& obj = sceneObjects[i];
            
            glActiveTexture(GL_TEXTURE0);
            if (obj.geometry.textureID > 0) glBindTexture(GL_TEXTURE_2D, obj.geometry.textureID);
            glUniform1i(glGetUniformLocation(shaderID, "tex_buffer"), 0);
            
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, obj.position);
            
            // Rotação inicial para posicionar o objeto virado para a frente (câmera)
            model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            
            if (rotateX)
                model = glm::rotate(model, angle, glm::vec3(1.0f, 0.0f, 0.0f));
            else if (rotateY)
                model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));
            else if (rotateZ)
                model = glm::rotate(model, angle, glm::vec3(0.0f, 0.0f, 1.0f));
                
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glBindVertexArray(obj.geometry.VAO);
            glDrawArrays(GL_TRIANGLES, 0, obj.geometry.vertexCount);
            glBindVertexArray(0);
        }

        // Renderização dos pontos de controle da trajetória
        renderTrajectoryPoints(sceneObjects[selectedObjectIndex].trajectory, shaderID, view, projection);

        // Troca de buffers
        glfwSwapBuffers(window);
    }

    // Limpeza
    for (auto& obj : sceneObjects)
    {
        glDeleteVertexArrays(1, &obj.geometry.VAO);
    }
    glfwTerminate();
    return 0;
}

// Função de callback de teclado - só pode ter uma instância (deve ser estática se
// estiver dentro de uma classe) - É chamada sempre que uma tecla for pressionada
// ou solta via GLFW

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

		if (key == GLFW_KEY_X && action == GLFW_PRESS)
		{
			rotateX = true;
			rotateY = false;
			rotateZ = false;
		}

		if (key == GLFW_KEY_Y && action == GLFW_PRESS)
		{
			rotateX = false;
			rotateY = true;
			rotateZ = false;
		}

		if (key == GLFW_KEY_Z && action == GLFW_PRESS)
		{
			rotateX = false;
			rotateY = false;
			rotateZ = true;
		}
		
		// Controles de trajetória
		if (key == GLFW_KEY_T && action == GLFW_PRESS)
		{
			trajectoryMode = !trajectoryMode;
			cout << "Modo trajetória: " << (trajectoryMode ? "ATIVADO" : "DESATIVADO") << endl;
			cout << "Objeto selecionado: " << sceneObjects[selectedObjectIndex].name << endl;
			
			// Habilitar/desabilitar cursor do mouse
			if (trajectoryMode)
			{
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
				cout << "Cursor do mouse habilitado para adicionar pontos" << endl;
			}
			else
			{
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
				cout << "Cursor do mouse desabilitado" << endl;
			}
		}
		
		if (key == GLFW_KEY_P && action == GLFW_PRESS && trajectoryMode)
		{
			// Adiciona ponto de controle na posição atual da câmera
			glm::vec3 pointPos = camera.position + camera.front * 2.0f; // 2 unidades à frente da câmera
			sceneObjects[selectedObjectIndex].trajectory.addControlPoint(pointPos, 2.0f);
			cout << "Ponto adicionado em: (" << pointPos.x << ", " << pointPos.y << ", " << pointPos.z << ")" << endl;
			cout << "Total de pontos: " << sceneObjects[selectedObjectIndex].trajectory.getPointCount() << endl;
		}
		
		if (key == GLFW_KEY_SPACE && action == GLFW_PRESS && trajectoryMode)
		{
			// Inicia/para a trajetória
			if (sceneObjects[selectedObjectIndex].trajectory.isRunning())
			{
				sceneObjects[selectedObjectIndex].trajectory.stop();
				cout << "Trajetória parada" << endl;
			}
			else
			{
				sceneObjects[selectedObjectIndex].trajectory.start();
				cout << "Trajetória iniciada" << endl;
			}
		}
		
		if (key == GLFW_KEY_C && action == GLFW_PRESS && trajectoryMode)
		{
			// Limpa a trajetória
			sceneObjects[selectedObjectIndex].trajectory.clearControlPoints();
			cout << "Trajetória limpa" << endl;
		}
		
		if (key == GLFW_KEY_S && action == GLFW_PRESS && trajectoryMode)
		{
			// Salva a trajetória
			string filename = "trajectory_" + sceneObjects[selectedObjectIndex].name + ".txt";
			if (sceneObjects[selectedObjectIndex].trajectory.saveToFile(filename))
			{
				cout << "Trajetória salva em: " << filename << endl;
			}
			else
			{
				cout << "Erro ao salvar trajetória" << endl;
			}
		}
		
		if (key == GLFW_KEY_L && action == GLFW_PRESS && trajectoryMode)
		{
			// Carrega a trajetória
			string filename = "trajectory_" + sceneObjects[selectedObjectIndex].name + ".txt";
			if (sceneObjects[selectedObjectIndex].trajectory.loadFromFile(filename))
			{
				cout << "Trajetória carregada de: " << filename << endl;
			}
			else
			{
				cout << "Erro ao carregar trajetória" << endl;
			}
		}
		
		if (key == GLFW_KEY_O && action == GLFW_PRESS)
		{
			// Seleciona próximo objeto
			selectedObjectIndex = (selectedObjectIndex + 1) % sceneObjects.size();
			cout << "Objeto selecionado: " << sceneObjects[selectedObjectIndex].name << endl;
		}
		
		if (key == GLFW_KEY_V && action == GLFW_PRESS)
		{
			// Mostra/esconde pontos de trajetória
			showTrajectoryPoints = !showTrajectoryPoints;
			cout << "Visualização de pontos: " << (showTrajectoryPoints ? "ATIVADA" : "DESATIVADA") << endl;
		}
		
		// Exemplo de trajetória circular
		if (key == GLFW_KEY_1 && action == GLFW_PRESS && trajectoryMode)
		{
			auto& trajectory = sceneObjects[selectedObjectIndex].trajectory;
			trajectory.clearControlPoints();
			
			// Criar trajetória circular
			float radius = 3.0f;
			int numPoints = 8;
			for (int i = 0; i < numPoints; ++i)
			{
				float angle = (2.0f * M_PI * i) / numPoints;
				glm::vec3 pos(radius * cos(angle), 0.0f, radius * sin(angle));
				trajectory.addControlPoint(pos, 1.0f);
			}
			cout << "Trajetória circular criada com " << numPoints << " pontos" << endl;
		}
		
		// Exemplo de trajetória em forma de quadrado
		if (key == GLFW_KEY_2 && action == GLFW_PRESS && trajectoryMode)
		{
			auto& trajectory = sceneObjects[selectedObjectIndex].trajectory;
			trajectory.clearControlPoints();
			
			// Criar trajetória quadrada
			float size = 2.0f;
			trajectory.addControlPoint(glm::vec3(-size, 0.0f, -size), 2.0f);
			trajectory.addControlPoint(glm::vec3(size, 0.0f, -size), 2.0f);
			trajectory.addControlPoint(glm::vec3(size, 0.0f, size), 2.0f);
			trajectory.addControlPoint(glm::vec3(-size, 0.0f, size), 2.0f);
			
			cout << "Trajetória quadrada criada" << endl;
		}
		
		// Exemplo de trajetória em forma de triângulo
		if (key == GLFW_KEY_3 && action == GLFW_PRESS && trajectoryMode)
		{
			auto& trajectory = sceneObjects[selectedObjectIndex].trajectory;
			trajectory.clearControlPoints();
			
			// Criar trajetória triangular
			float size = 2.0f;
			trajectory.addControlPoint(glm::vec3(0.0f, 0.0f, size), 1.5f);
			trajectory.addControlPoint(glm::vec3(-size, 0.0f, -size), 1.5f);
			trajectory.addControlPoint(glm::vec3(size, 0.0f, -size), 1.5f);
			
			cout << "Trajetória triangular criada" << endl;
		}
		
		// Mostrar informações da trajetória atual
		if (key == GLFW_KEY_I && action == GLFW_PRESS && trajectoryMode)
		{
			const auto& trajectory = sceneObjects[selectedObjectIndex].trajectory;
			cout << "=== Informações da Trajetória ===" << endl;
			cout << "Objeto: " << sceneObjects[selectedObjectIndex].name << endl;
			cout << "Número de pontos: " << trajectory.getPointCount() << endl;
			cout << "Status: " << (trajectory.isRunning() ? "ATIVA" : "PARADA") << endl;
			
			const auto& points = trajectory.getControlPoints();
			for (size_t i = 0; i < points.size(); ++i)
			{
				cout << "Ponto " << i << ": (" << points[i].position.x << ", " 
					 << points[i].position.y << ", " << points[i].position.z 
					 << ") - Tempo: " << points[i].time << "s" << endl;
			}
			cout << "================================" << endl;
		}
    }
}

// Função de callback para movimento do mouse
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // Invertido porque o eixo Y das coordenadas da tela vai de cima para baixo

    lastX = xpos;
    lastY = ypos;

    camera.processMouseMovement(xoffset, yoffset);
}

// Função de callback para clique do mouse
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && trajectoryMode)
    {
        // Adiciona ponto de controle na posição atual da câmera
        glm::vec3 pointPos = camera.position + camera.front * 3.0f; // 3 unidades à frente da câmera
        sceneObjects[selectedObjectIndex].trajectory.addControlPoint(pointPos, 2.0f);
        cout << "Ponto adicionado com mouse em: (" << pointPos.x << ", " << pointPos.y << ", " << pointPos.z << ")" << endl;
        cout << "Total de pontos: " << sceneObjects[selectedObjectIndex].trajectory.getPointCount() << endl;
    }
}

// Função para processar input contínuo (movimento da câmera)
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.processKeyboard("FORWARD", deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.processKeyboard("BACKWARD", deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.processKeyboard("LEFT", deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.processKeyboard("RIGHT", deltaTime);
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        camera.processKeyboard("UP", deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        camera.processKeyboard("DOWN", deltaTime);
}

// Esta função está basntante hardcoded - objetivo é compilar e "buildar" um programa de
// shader simples e único neste exemplo de código
// O código fonte do vertex e fragment shader está nos arrays vertexShaderSource e
// fragmentShader source no iniçio deste arquivo
// A função retorna o identificador do programa de shader
int setupShader()
{
	// Vertex shader
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);
	// Checando erros de compilação (exibição via log no terminal)
	GLint success;
	GLchar infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
	}
	// Fragment shader
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);
	// Checando erros de compilação (exibição via log no terminal)
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
	}
	// Linkando os shaders e criando o identificador do programa de shader
	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	// Checando por erros de linkagem
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
	}
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return shaderProgram;
}

// Esta função está bastante harcoded - objetivo é criar os buffers que armazenam a 
// geometria de um triângulo
// Apenas atributo coordenada nos vértices
// 1 VBO com as coordenadas, VAO com apenas 1 ponteiro para atributo
// A função retorna o identificador do VAO
int setupGeometry()
{
	GLfloat vertices[] = {
		// Frente (vermelho)
		-0.5f, -0.5f,  0.5f, 1, 0, 0, 0.0f, 0.0f,
		0.5f, -0.5f,  0.5f, 1, 0, 0, 1.0f, 0.0f,
		0.5f,  0.5f,  0.5f, 1, 0, 0, 1.0f, 1.0f,
		-0.5f, -0.5f,  0.5f, 1, 0, 0, 0.0f, 0.0f,
		0.5f,  0.5f,  0.5f, 1, 0, 0, 1.0f, 1.0f,
		-0.5f,  0.5f,  0.5f, 1, 0, 0, 0.0f, 1.0f,

		// Trás (verde)
		-0.5f, -0.5f, -0.5f, 0, 1, 0, 0.0f, 0.0f,
		-0.5f,  0.5f, -0.5f, 0, 1, 0, 1.0f, 0.0f,
		0.5f,  0.5f, -0.5f, 0, 1, 0, 1.0f, 1.0f,
		-0.5f, -0.5f, -0.5f, 0, 1, 0, 0.0f, 0.0f,
		0.5f,  0.5f, -0.5f, 0, 1, 0, 1.0f, 1.0f,
		0.5f, -0.5f, -0.5f, 0, 1, 0, 0.0f, 1.0f,

		// Direita (azul)
		0.5f, -0.5f,  0.5f, 0, 0, 1, 0.0f, 0.0f,
		0.5f, -0.5f, -0.5f, 0, 0, 1, 1.0f, 0.0f,
		0.5f,  0.5f, -0.5f, 0, 0, 1, 1.0f, 1.0f,
		0.5f, -0.5f,  0.5f, 0, 0, 1, 0.0f, 0.0f,
		0.5f,  0.5f, -0.5f, 0, 0, 1, 1.0f, 1.0f,
		0.5f,  0.5f,  0.5f, 0, 0, 1, 0.0f, 1.0f,

		// Esquerda (amarelo)
		-0.5f, -0.5f,  0.5f, 1, 1, 0, 0.0f, 0.0f,
		-0.5f,  0.5f,  0.5f, 1, 1, 0, 1.0f, 0.0f,
		-0.5f,  0.5f, -0.5f, 1, 1, 0, 1.0f, 1.0f,
		-0.5f, -0.5f,  0.5f, 1, 1, 0, 0.0f, 0.0f,
		-0.5f,  0.5f, -0.5f, 1, 1, 0, 1.0f, 1.0f,
		-0.5f, -0.5f, -0.5f, 1, 1, 0, 0.0f, 1.0f,

		// Topo (magenta)
		-0.5f,  0.5f,  0.5f, 1, 0, 1, 0.0f, 0.0f,
		0.5f,  0.5f,  0.5f, 1, 0, 1, 1.0f, 0.0f,
		0.5f,  0.5f, -0.5f, 1, 0, 1, 1.0f, 1.0f,
		-0.5f,  0.5f,  0.5f, 1, 0, 1, 0.0f, 0.0f,
		0.5f,  0.5f, -0.5f, 1, 0, 1, 1.0f, 1.0f,
		-0.5f,  0.5f, -0.5f, 1, 0, 1, 0.0f, 1.0f,

		// Fundo (ciano)
		-0.5f, -0.5f,  0.5f, 0, 1, 1, 0.0f, 0.0f,
		-0.5f, -0.5f, -0.5f, 0, 1, 1, 1.0f, 0.0f,
		0.5f, -0.5f, -0.5f, 0, 1, 1, 1.0f, 1.0f,
		-0.5f, -0.5f,  0.5f, 0, 1, 1, 0.0f, 0.0f,
		0.5f, -0.5f, -0.5f, 0, 1, 1, 1.0f, 1.0f,
		0.5f, -0.5f,  0.5f, 0, 1, 1, 0.0f, 1.0f,
	};

	GLuint VBO, VAO;

	//Geração do identificador do VBO
	glGenBuffers(1, &VBO);

	//Faz a conexão (vincula) do buffer como um buffer de array
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	//Envia os dados do array de floats para o buffer da OpenGl
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	//Geração do identificador do VAO (Vertex Array Object)
	glGenVertexArrays(1, &VAO);

	// Vincula (bind) o VAO primeiro, e em seguida  conecta e seta o(s) buffer(s) de vértices
	// e os ponteiros para os atributos 
	glBindVertexArray(VAO);
	
	//Para cada atributo do vertice, criamos um "AttribPointer" (ponteiro para o atributo), indicando: 
	// Localização no shader * (a localização dos atributos devem ser correspondentes no layout especificado no vertex shader)
	// Numero de valores que o atributo tem (por ex, 3 coordenadas xyz) 
	// Tipo do dado
	// Se está normalizado (entre zero e um)
	// Tamanho em bytes 
	// Deslocamento a partir do byte zero 
	
	//Atributo posição (x, y, z)
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);

	//Atributo cor (r, g, b)
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3*sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	// Atributo de textura (s, t)
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
	glEnableVertexAttribArray(2);

	// Observe que isso é permitido, a chamada para glVertexAttribPointer registrou o VBO como o objeto de buffer de vértice 
	// atualmente vinculado - para que depois possamos desvincular com segurança
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Desvincula o VAO (é uma boa prática desvincular qualquer buffer ou array para evitar bugs medonhos)
	glBindVertexArray(0);

	return VAO;
}

// Função para carregar textura
int loadTexture(const string& path)
{
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height, nrChannels;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);

    if (data)
    {
        GLenum format = (nrChannels == 3) ? GL_RGB : GL_RGBA;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        cerr << "Failed to load texture: " << path << endl;
    }

    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);

    return texID;
}

// Função para carregar arquivo OBJ
bool loadObject(
	const char* path,
	std::vector<glm::vec3>& out_vertices,
	std::vector<glm::vec2>& out_uvs,
	std::vector<glm::vec3>& out_normals)
{
	std::ifstream file(path);

	if (!file)
	{
			std::cerr << "Failed to open file: " << path << std::endl;
			return false;
	}

	std::vector<unsigned int> vertexIndices, uvIndices, normalIndices;
	std::vector<glm::vec3> temp_vertices;
	std::vector<glm::vec2> temp_uvs;
	std::vector<glm::vec3> temp_normals;

	std::string line;

	while (std::getline(file, line))
	{
			std::istringstream iss(line);
			std::string type;
			iss >> type;

			if (type == "v") 
			{
					glm::vec3 vertex;
					iss >> vertex.x >> vertex.y >> vertex.z;
					temp_vertices.push_back(vertex);
			}
			else if (type == "vt") 
			{
					glm::vec2 uv;
					iss >> uv.x >> uv.y;
					temp_uvs.push_back(uv);
			}
			else if (type == "vn") 
			{
					glm::vec3 normal;
					iss >> normal.x >> normal.y >> normal.z;
					temp_normals.push_back(normal);
			}
			else if (type == "f")
			{
					unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];
					char slash;

					for (int i = 0; i < 3; ++i)
					{
							iss >> vertexIndex[i] >> slash >> uvIndex[i] >> slash >> normalIndex[i];
							vertexIndices.push_back(vertexIndex[i]);
							uvIndices.push_back(uvIndex[i]);
							normalIndices.push_back(normalIndex[i]);
					}
			}
			else if (type == "mtllib")
			{
					iss >> mtlFilePath;
			}
	}

	for (unsigned int i = 0; i < vertexIndices.size(); ++i)
	{
			unsigned int vertexIndex = vertexIndices[i];
			unsigned int uvIndex = uvIndices[i];
			unsigned int normalIndex = normalIndices[i];

			glm::vec3 vertex = temp_vertices[vertexIndex - 1];
			glm::vec2 uv = temp_uvs[uvIndex - 1];
			glm::vec3 normal = temp_normals[normalIndex - 1];

			out_vertices.push_back(vertex);
			out_uvs.push_back(uv);
			out_normals.push_back(normal);
	}

	file.close();

	return true;
}

// Função para configurar geometria a partir de arquivo OBJ
Geometry setupGeometryFromFile(const char* filepath)
{
    std::vector<GLfloat> vertices;
    std::vector<glm::vec3> vert;
    std::vector<glm::vec2> uvs;
    std::vector<glm::vec3> normals;

    loadObject(filepath, vert, uvs, normals);

    vertices.reserve(vert.size() * 11); // 11 componentes: pos(3) + normal(3) + cor(3) + tex(2)
    for (size_t i = 0; i < vert.size(); ++i)
    {
        vertices.insert(vertices.end(), {
            vert[i].x, vert[i].y, vert[i].z,           // posição
            normals[i].x, normals[i].y, normals[i].z,   // normal
            1.0f, 0.0f, 0.0f,                          // cor (vermelho)
            uvs[i].x, uvs[i].y                         // coordenadas de textura
        });
    }

    GLuint VBO, VAO;

    glGenBuffers(1, &VBO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);

    glGenVertexArrays(1, &VAO);

    glBindVertexArray(VAO);

    // Atributo posição (x, y, z)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    // Atributo cor (r, g, b)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    // Atributo de textura (s, t)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(9 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);

    // Atributo normal (nx, ny, nz)
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(3);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    Geometry geom;
    geom.VAO = VAO;
    geom.vertexCount = vertices.size() / 11; // Corrigido para 11 componentes por vértice

    string basePath = string(filepath).substr(0, string(filepath).find_last_of("/"));
    string mtlPath = basePath + "/" + mtlFilePath;
    string textureFile = loadMTL(mtlPath);

    if (!textureFile.empty())
    {
        string fullTexturePath = basePath + "/" + textureFile;
        geom.textureID = loadTexture(fullTexturePath);
        geom.textureFilePath = fullTexturePath;
    }

    return geom;
}

// Função para carregar arquivo MTL
string loadMTL(const string& path)
{
    ifstream mtlFile(path);
    if (!mtlFile)
    {
        cerr << "Failed to open MTL file: " << path << endl;
        return "";
    }

    string line, texturePath;
    while (getline(mtlFile, line))
    {
        istringstream iss(line);
        string keyword;
        iss >> keyword;

        if (keyword == "map_Kd")
        {
            iss >> texturePath;
        }
        else if (keyword == "Ka")
        {
            iss >> ambientColor.r >> ambientColor.g >> ambientColor.b;
        }
        else if (keyword == "Kd")
        {
            iss >> diffuseColor.r >> diffuseColor.g >> diffuseColor.b;
        }
        else if (keyword == "Ks")
        {
            iss >> specularColor.r >> specularColor.g >> specularColor.b;
        }
        else if (keyword == "Ns")
        {
            iss >> shininess;
        }
        else if (keyword == "Ke")
        {
            iss >> emissiveColor.r >> emissiveColor.g >> emissiveColor.b;
        }
    }
    mtlFile.close();

    if (texturePath.empty())
    {
        cerr << "No diffuse texture found in MTL file: " << path << endl;
    }
    return texturePath;
}

// Função para renderizar pontos de controle da trajetória
void renderTrajectoryPoints(const Trajectory& trajectory, GLuint shaderID, 
                           const glm::mat4& view, const glm::mat4& projection)
{
    if (!showTrajectoryPoints)
        return;

    // Criar geometria temporária para os pontos de controle
    static GLuint controlPointVAO = 0;
    if (controlPointVAO == 0)
    {
        controlPointVAO = createControlPointGeometry();
    }

    // Obter localização dos uniforms
    GLint modelLoc = glGetUniformLocation(shaderID, "model");
    GLint viewLoc = glGetUniformLocation(shaderID, "view");
    GLint projLoc = glGetUniformLocation(shaderID, "projection");

    // Configurar matrizes
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Renderizar cada ponto de controle
    const auto& controlPoints = trajectory.getControlPoints();
    for (size_t i = 0; i < controlPoints.size(); ++i)
    {
        const auto& point = controlPoints[i];
        
        // Criar matriz de modelo para o ponto
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, point.position);
        // Não precisa de escala pois já é pequeno
        
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        
        // Renderizar o ponto (cubo vermelho simples)
        glBindVertexArray(controlPointVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36); // 36 vértices para um cubo
        glBindVertexArray(0);
    }
}

// Função para criar geometria de pontos de controle
GLuint createControlPointGeometry()
{
    // Geometria simples de um cubo pequeno (apenas posição e cor)
    GLfloat vertices[] = {
        // Frente (vermelho)
        -0.05f, -0.05f,  0.05f, 1.0f, 0.0f, 0.0f,
        0.05f, -0.05f,  0.05f, 1.0f, 0.0f, 0.0f,
        0.05f,  0.05f,  0.05f, 1.0f, 0.0f, 0.0f,
        -0.05f, -0.05f,  0.05f, 1.0f, 0.0f, 0.0f,
        0.05f,  0.05f,  0.05f, 1.0f, 0.0f, 0.0f,
        -0.05f,  0.05f,  0.05f, 1.0f, 0.0f, 0.0f,

        // Trás (vermelho)
        -0.05f, -0.05f, -0.05f, 1.0f, 0.0f, 0.0f,
        -0.05f,  0.05f, -0.05f, 1.0f, 0.0f, 0.0f,
        0.05f,  0.05f, -0.05f, 1.0f, 0.0f, 0.0f,
        -0.05f, -0.05f, -0.05f, 1.0f, 0.0f, 0.0f,
        0.05f,  0.05f, -0.05f, 1.0f, 0.0f, 0.0f,
        0.05f, -0.05f, -0.05f, 1.0f, 0.0f, 0.0f,

        // Direita (vermelho)
        0.05f, -0.05f,  0.05f, 1.0f, 0.0f, 0.0f,
        0.05f, -0.05f, -0.05f, 1.0f, 0.0f, 0.0f,
        0.05f,  0.05f, -0.05f, 1.0f, 0.0f, 0.0f,
        0.05f, -0.05f,  0.05f, 1.0f, 0.0f, 0.0f,
        0.05f,  0.05f, -0.05f, 1.0f, 0.0f, 0.0f,
        0.05f,  0.05f,  0.05f, 1.0f, 0.0f, 0.0f,

        // Esquerda (vermelho)
        -0.05f, -0.05f,  0.05f, 1.0f, 0.0f, 0.0f,
        -0.05f,  0.05f,  0.05f, 1.0f, 0.0f, 0.0f,
        -0.05f,  0.05f, -0.05f, 1.0f, 0.0f, 0.0f,
        -0.05f, -0.05f,  0.05f, 1.0f, 0.0f, 0.0f,
        -0.05f,  0.05f, -0.05f, 1.0f, 0.0f, 0.0f,
        -0.05f, -0.05f, -0.05f, 1.0f, 0.0f, 0.0f,

        // Topo (vermelho)
        -0.05f,  0.05f,  0.05f, 1.0f, 0.0f, 0.0f,
        0.05f,  0.05f,  0.05f, 1.0f, 0.0f, 0.0f,
        0.05f,  0.05f, -0.05f, 1.0f, 0.0f, 0.0f,
        -0.05f,  0.05f,  0.05f, 1.0f, 0.0f, 0.0f,
        0.05f,  0.05f, -0.05f, 1.0f, 0.0f, 0.0f,
        -0.05f,  0.05f, -0.05f, 1.0f, 0.0f, 0.0f,

        // Fundo (vermelho)
        -0.05f, -0.05f,  0.05f, 1.0f, 0.0f, 0.0f,
        -0.05f, -0.05f, -0.05f, 1.0f, 0.0f, 0.0f,
        0.05f, -0.05f, -0.05f, 1.0f, 0.0f, 0.0f,
        -0.05f, -0.05f,  0.05f, 1.0f, 0.0f, 0.0f,
        0.05f, -0.05f, -0.05f, 1.0f, 0.0f, 0.0f,
        0.05f, -0.05f,  0.05f, 1.0f, 0.0f, 0.0f,
    };

    GLuint VBO, VAO;

    //Geração do identificador do VBO
    glGenBuffers(1, &VBO);

    //Faz a conexão (vincula) do buffer como um buffer de array
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    //Envia os dados do array de floats para o buffer da OpenGl
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    //Geração do identificador do VAO (Vertex Array Object)
    glGenVertexArrays(1, &VAO);

    // Vincula (bind) o VAO primeiro, e em seguida  conecta e seta o(s) buffer(s) de vértices
    // e os ponteiros para os atributos 
    glBindVertexArray(VAO);
    
    //Atributo posição (x, y, z)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    //Atributo cor (r, g, b)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3*sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    // Observe que isso é permitido, a chamada para glVertexAttribPointer registrou o VBO como o objeto de buffer de vértice 
    // atualmente vinculado - para que depois possamos desvincular com segurança
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Desvincula o VAO (é uma boa prática desvincular qualquer buffer ou array para evitar bugs medonhos)
    glBindVertexArray(0);

    return VAO;
}