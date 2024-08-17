// This has been adapted from the Vulkan tutorial
#define GLM_ENABLE_EXPERIMENTAL
#define NSHIP 16
#include "Starter.hpp"
#include "modules/Scene.hpp"
#include "modules/TextMaker.hpp"

// The uniform buffer objects data structures
// Remember to use the correct alignas(...) value
//        float : alignas(4)
//        vec2  : alignas(8)
//        vec3  : alignas(16)
//        vec4  : alignas(16)
//        mat3  : alignas(16)
//        mat4  : alignas(16)

//COLLIDER
struct ColliderQuad{
	glm::vec3 vertice1;
	glm::vec3 vertice2;
	glm::vec3 vertice3;
	glm::vec3 vertice4;
/*
*	ver1----ver2
* 	 |		 |
*	 |		 |
*	ver4----ver3
*/
};


/********************Uniform Blocks********************/
//UNIFORM BUFFER - BLINN -
#define NLIGHTS 4

//TextAlignment alignment = Menu;

const char* cloudState[2] = { "ON", "OFF" };
const char* headlightState[2] = { "ON", "OFF" };
const char* bdrfState[2] = { "Blinn", "Toon" };
const char* bdrf = bdrfState[0];

std::vector<SingleText> outText = {
	{2, {"Menu", "Press 4 To Start", "", "", ""}, 0, 0, 0},
	{1, {"Press 4 To See Commands","","", "",""}, 0, 0, 0},
	{5, {"HeadLights: 1","BDRF: 2","Clouds: 3","P to change camera", "Press 4 To Restart"}, 0, 0, 0},
};

std::vector<SingleText> outText2 = {
	{1, {"","","", ""}, 0, 0, 0},
	{1, {"HeadLights(1): ON", "", ""}, 0, 0, 0},
	{1, {"HeadLights(1): OFF","","", ""}, 0, 0, 0},
};

std::vector<SingleText> outText3 = {
	{1, {"BDRF(2): Blinn", "", ""}, 0, 0, 1},
	{1, {"BDRF(2): Toon","","", ""}, 0, 0, 1},
};

std::vector<SingleText> outText4 = {
	{1, {"Clouds(3): ON", "", ""}, 0, 0, 2},
	{1, {"Clouds(3): OFF","","", ""}, 0, 0, 2},
};



struct BlinnUniformBufferObject {
	alignas(16) glm::mat4 mvpMat;
	alignas(16) glm::mat4 mMat;
	alignas(16) glm::mat4 nMat;
};

struct BlinnMatParUniformBufferObject {
	alignas(4)  float Power;
	alignas(4)  float isCar;
};

//UNIFORM BUFFER - GLOBAL -
struct GlobalUniformBufferObject {
	alignas(16) glm::vec3 lightDir[NLIGHTS];
	alignas(16) glm::vec4 lightColor[NLIGHTS];
	alignas(16) glm::vec3 eyePos;
	alignas(16) glm::vec3 lightPos[NLIGHTS];
	alignas(4) float cosIn;
	alignas(4) float cosOut;
	alignas(4) float lightOn;
	alignas(4) float BDRFState;
};

//SUN
struct EmissionUniformBufferObject {
	alignas(16) glm::mat4 mvpMat;
};

struct SunParUniformBufferObject {
	alignas(4) float sinSun;
	alignas(4) float isMoon;
};

//SKY
struct skyBoxUniformBufferObject {
	alignas(16) glm::mat4 mvpMat;
};

struct SkyMatParUniformBufferObject {
	alignas(4)  float Ang;
	alignas(4)  float ShowCloud;
	alignas(4)  float sinSun;
};

/********************The vertices data structures********************/

//VERTICES - BLINN -
struct BlinnVertex {
	glm::vec3 pos;
	glm::vec3 norm;
	glm::vec2 UV;
};
struct EmissionVertex {
	glm::vec3 pos;
	glm::vec2 UV;
};




// MAIN ! 
class MeshLoader : public BaseProject {
	protected:

	//SCENE
		int currScene = 0;

	//SYSTEM PARAMETERS
		bool debounce = false;	//avoid multiple key press
		int curDebounce = 0;
		SceneProject scene;
		ColliderQuad colObstacle[4];

	//CAMERA PARAMETERS
		float Ar;	// Current aspect ratio
		int cameraType = 0;	//0 = camera look at, 1 = camera look in direction
		
		//CONSTANTS
		const float FOVy = glm::radians(90.0f);
		const float nearPlane = 0.1f;
		const float farPlane = 500.0f;
		
		const float ROT_SPEED_CAMERA = glm::radians(120.0f);
		const float fixedCamDist = 10.0f;	//distanza fissa del target
		const float PilotDist = 0.0f;		//distanza del pilota dal target
		const float lambdaCam = 10.0f;		//costante per l'esponenziale della camera

	//CAR PARAMETERS
		float carYaw = 0.0f;
		float decayFactor = 0.0f;
		float turningRadius = 0.0f;
		float steeringAngle = 0.0f;

		glm::vec3 carDirection;
		glm::vec3 Pos = glm::vec3(-15.0f, 0.0f, 30.0f);
		glm::vec3 Vel = glm::vec3(0.0f, 0.0f, 0.0f);

		//MATRICES FOR THE MODEL
		glm::mat4 initialRotation;
		glm::mat4 rotationMatrix;
		glm::mat4 translationMatrix;

		//CONSTANTS
		const float R = 2.5f;	//lunghezza dell'auto
		const float W = 1.25f;	//larghezza dell'auto
		const float MAX_SPEED = 10.0f;
		const float ACC_CAR = 20.0f;
		const float FRICTION = 5.0f;
		const float ROT_SPEED_CAR = glm::radians(120.0f) * 10.0f;

	//HEADLIGHTS
		float lightOn;
	//BDRF
		float BDRFState;
	//Clouds
		float showClouds;
	//GamePause
		float gamePaused = 0;
			
	//MATRICES
		glm::mat4 View;
		glm::mat4 baseTr = glm::mat4(1);

	//********************DESCRIPTOR SET LAYOUT ["classes" of what will be passed to the shaders]
	DescriptorSetLayout DSLGlobal;
	DescriptorSetLayout DSLBlinn;	// For Blinn Objects
	DescriptorSetLayout DSLEmission;
	DescriptorSetLayout DSLSunPar;
	DescriptorSetLayout DSLScene;
	DescriptorSetLayout DSLskyBox;	// For skyBox
	DescriptorSetLayout DSLskyBoxPar;	// For skyBox parameters

	//********************VERTEX DESCRIPTOR
	VertexDescriptor VDBlinn;
	VertexDescriptor VDEmission;
	/*VertexDescriptor VDskyBox;*/

	//********************PIPELINES [Shader couples]
	Pipeline PBlinn;
	Pipeline PEmission;
	Pipeline PScene;
	Pipeline PskyBox;

	// Models, textures and Descriptors (values assigned to the uniforms)
	// Please note that Model objects depends on the corresponding vertex structure
	//********************MODELS
	Model MCar;
	Model Mship;

	Model Msun;
	Texture Tsun;
	DescriptorSet DSsun, DSsunPar;

	Model Mmoon;
	Texture Tmoon;
	DescriptorSet DSmoon, DSmoonPar;

	Model MskyBox;
	Texture TskyBox, Tstars;
	DescriptorSet DSskyBox, DSskyBoxPar;

	//Model MFloor[1];
	//std::vector<std::array<float,6>> vertices_pos_floor[1];
	//DescriptorSet DSFloor;
	//Texture TFloor;

	//********************DESCRIPTOR SETS
	DescriptorSet DSCar;
	DescriptorSet DSGlobal;
	DescriptorSet DSship;

	//********************TEXTURES
	Texture TCar;
	Texture Tship;

	TextMaker txt;
	//TextMaker txt2;

	void setWindowParameters() {
		// window size, titile and initial background
		windowWidth = 800;
		windowHeight = 600;
		windowTitle = "Mesh Loader";
    	windowResizable = GLFW_TRUE;
		initialBackgroundColor = {0.0f, 0.005f, 0.0f, 1.0f};
		
		// Descriptor pool sizes
		DPSZs.uniformBlocksInPool = 97;//aumento di 2
		DPSZs.texturesInPool = 50;//aumentato di 1
		DPSZs.setsInPool = 53;//aumento di 1
		
		Ar = (float)windowWidth / (float)windowHeight;
	}
	
	// What to do when the window changes size
	void onWindowResize(int w, int h) {
		Ar = (float)w / (float)h;
	}
	
	void localInit() {

		lightOn = 1.0;
		BDRFState = 1.0;
		showClouds = 1.0;

		// Descriptor Layouts INITIALIZATION [what will be passed to the shaders]
		DSLGlobal.init(this, {
			{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(GlobalUniformBufferObject), 1}
		});

		DSLEmission.init(this, {
			{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(EmissionUniformBufferObject), 1},
			{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1}
		});

		DSLSunPar.init(this, {
			{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(SunParUniformBufferObject), 1},
		});

		DSLBlinn.init(this, {
			{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(BlinnUniformBufferObject), 1},
			{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1},
			{2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(BlinnMatParUniformBufferObject), 1}
		});
		
		DSLScene.init(this, {
			{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(BlinnUniformBufferObject), 1},
			{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1},
			{2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(BlinnMatParUniformBufferObject), 1}
		});

		DSLskyBox.init(this, {
			{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(skyBoxUniformBufferObject), 1},
			{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1},
			{2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1}
		});

		DSLskyBoxPar.init(this, {
			{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(SkyMatParUniformBufferObject), 1},
		});


		// Vertex descriptors INITIALIZATION
		VDBlinn.init(this, {
			{0, sizeof(BlinnVertex), VK_VERTEX_INPUT_RATE_VERTEX}
		}, {
			{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(BlinnVertex, pos),
					sizeof(glm::vec3), POSITION},
			{0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(BlinnVertex, norm),
					sizeof(glm::vec3), NORMAL},
			{0, 2, VK_FORMAT_R32G32_SFLOAT, offsetof(BlinnVertex, UV),
					sizeof(glm::vec2), UV}
		});
		

		VDEmission.init(this, {
			{0, sizeof(EmissionVertex), VK_VERTEX_INPUT_RATE_VERTEX}
		}, {
			{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(EmissionVertex, pos),
					sizeof(glm::vec3), POSITION},
			{0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(EmissionVertex, UV),
					sizeof(glm::vec2), UV}
		});


		// Pipelines INITIALIZATION [Shader couples]
		PEmission.init(this, &VDEmission, "shaders/EmissionVert.spv", "shaders/EmissionFrag.spv", { &DSLEmission, &DSLSunPar});
		PBlinn.init(this, &VDBlinn,  "shaders/CarVert.spv",    "shaders/CarFrag.spv", {&DSLGlobal, &DSLBlinn/*,&DSLSpot*/});
		PScene.init(this, &VDBlinn, "shaders/CarVert.spv", "shaders/CarFrag.spv", {&DSLGlobal, &DSLScene});
		PskyBox.init(this, &VDEmission, "shaders/SkyBoxVert.spv", "shaders/SkyBoxFrag.spv", { &DSLskyBox, &DSLskyBoxPar});
		PskyBox.setAdvancedFeatures(VK_COMPARE_OP_LESS_OR_EQUAL, VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, false);

		// Create models		
		MCar.init(this, &VDBlinn, "Models/Car.mgcg", MGCG);
		Mship.init(this, &VDBlinn, "models/X-WING-baker.obj", OBJ);
		Msun.init(this, &VDEmission, "models/Sphere.obj", OBJ);
		Mmoon.init(this, &VDEmission, "models/Sphere.obj", OBJ);
		MskyBox.init(this, &VDEmission, "models/SkyBoxCube.obj", OBJ);

		/*// Creates a mesh with direct enumeration of vertices and indices
		M4.vertices = {{{-6,-2,-6}, {0.0f,0.0f}}, {{-6,-2,6}, {0.0f,1.0f}},
					    {{6,-2,-6}, {1.0f,0.0f}}, {{ 6,-2,6}, {1.0f,1.0f}}};
		M4.indices = {0, 1, 2,    1, 3, 2};
		M4.initMesh(this, &VD);*/

		// Create the textures
		TCar.init(this, "textures/CarTexture.png");
		Tship.init(this, "textures/XwingColors.png");
		Tsun.init(this, "textures/2k_sun.jpg");
		Tmoon.init(this, "textures/moon.jfif");
		TskyBox.init(this, "textures/starmap_g4k.jpg");
		Tstars.init(this, "textures/2k_earth_clouds.jpg");

		//INITIALIZE THE SCENE
		scene.init(this, &VDBlinn, DSLBlinn, PBlinn, "modules/scene.json");
		// updates the text
		txt.init(this, &outText);
		//txt2.init(this, &outText2);


		//INITIALIZE THE COLLIDERS
		InitializeColliders();

		//CREATE THE MODEL FOR THE FLOOR
		/*
		TFloor.init(this, "textures/Textures_Food.png");
		MakeFloor(2.0, vertices_pos_floor[0], MFloor[0].indices);

		MFloor[0].vertices = std::vector<unsigned char>(vertices_pos_floor[0].size()*sizeof(BlinnVertex), 0);

		for(int i = 0; i < vertices_pos_floor[0].size(); i++) {
			BlinnVertex *V_vertex = (BlinnVertex *)(&(MFloor[0].vertices[i]));

			V_vertex->pos.x = vertices_pos_floor[0][i][0];
			V_vertex->pos.y = vertices_pos_floor[0][i][1];
			V_vertex->pos.z = vertices_pos_floor[0][i][2];
			V_vertex->UV.x = vertices_pos_floor[0][i][0];
			V_vertex->UV.y = vertices_pos_floor[0][i][2];
			V_vertex->norm.x = vertices_pos_floor[0][i][3];
			V_vertex->norm.y = vertices_pos_floor[0][i][4];
			V_vertex->norm.z = vertices_pos_floor[0][i][5];
		}

		MFloor[0].initMesh(this, &VDBlinn);
		*/
		
	}
	
	void pipelinesAndDescriptorSetsInit() {
		// This creates a new pipeline (with the current surface), using its shaders
		PBlinn.create();
		PEmission.create();
		PScene.create();
		PskyBox.create();

		// Here you define the data set
		DSsun.init(this, &DSLEmission, { &Tsun });
		DSsunPar.init(this, &DSLSunPar, {});
		DSmoon.init(this, &DSLEmission, { &Tmoon });
		DSmoonPar.init(this, &DSLSunPar, {});
		DSGlobal.init(this, &DSLGlobal, {});
		DSship.init(this, &DSLBlinn, {&Tship});
		DSCar.init(this, &DSLBlinn, {&TCar});
		DSskyBox.init(this, &DSLskyBox, { &TskyBox, &Tstars });
		DSskyBoxPar.init(this, &DSLskyBoxPar, {});

		//floor
		//DSFloor.init(this, &DSLBlinn, {&TFloor});
		
		//scene pipelines and descriptor sets
		scene.pipelinesAndDescriptorSetsInit(DSLBlinn);
		txt.pipelinesAndDescriptorSetsInit();
		//txt2.pipelinesAndDescriptorSetsInit();
		
	}

	void pipelinesAndDescriptorSetsCleanup() {
		// Cleanup pipelines
		PBlinn.cleanup();
		PEmission.cleanup();
		PScene.cleanup();
		PskyBox.cleanup();

		DSGlobal.cleanup();
		DSCar.cleanup();
		DSship.cleanup();
		DSsun.cleanup();
		DSsunPar.cleanup();
		DSmoon.cleanup();
		DSmoonPar.cleanup();
		DSskyBox.cleanup();
		DSskyBoxPar.cleanup();
		//DSFloor.cleanup();
		
		//SCENE CLEANUP
		scene.pipelinesAndDescriptorSetsCleanup();
		txt.pipelinesAndDescriptorSetsCleanup();
		//txt2.pipelinesAndDescriptorSetsCleanup();
	}

	void localCleanup() {
		TCar.cleanup();
		MCar.cleanup();

		Tship.cleanup();
		Mship.cleanup();

		Tsun.cleanup();
		Msun.cleanup();

		Tmoon.cleanup();
		Mmoon.cleanup();

		TskyBox.cleanup();
		Tstars.cleanup();
		MskyBox.cleanup();
		
		// Cleanup descriptor set layouts
		DSLGlobal.cleanup();
		DSLBlinn.cleanup();
		DSLEmission.cleanup();
		DSLSunPar.cleanup();
		DSLScene.cleanup();
		DSLskyBox.cleanup();
		DSLskyBoxPar.cleanup();

		//SCENE CLEANUP
		scene.localCleanup();
		txt.localCleanup();
		//txt2.localCleanup();

		//FLOOR
		//TFloor.cleanup();
		//MFloor[0].cleanup();
		
		// Destroies the pipelines
		PBlinn.destroy();
		PEmission.destroy();
		PScene.destroy();
		PskyBox.destroy();
	}
	
	void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage) {
		if (currScene != 0) {
			PBlinn.bind(commandBuffer);
			Mship.bind(commandBuffer);
			DSGlobal.bind(commandBuffer, PBlinn, 0, currentImage);	// The Global Descriptor Set (Set 0)
			DSship.bind(commandBuffer, PBlinn, 1, currentImage);
			vkCmdDrawIndexed(commandBuffer,
				static_cast<uint32_t>(Mship.indices.size()), 1, 0, 0, 0);

			scene.populateCommandBuffer(commandBuffer, currentImage, PBlinn, DSGlobal);

			MCar.bind(commandBuffer);
			DSCar.bind(commandBuffer, PBlinn, 1, currentImage);
			vkCmdDrawIndexed(commandBuffer,
				static_cast<uint32_t>(MCar.indices.size()), 1, 0, 0, 0);
		//POPULATE SCENE
		scene.populateCommandBuffer(commandBuffer, currentImage, PBlinn, DSGlobal);
		//FLOOR
		//MFloor[0].bind(commandBuffer);
		//DSGlobal.bind(commandBuffer, PBlinn, 0, currentImage);
		//DSFloor.bind(commandBuffer, PBlinn, 1, currentImage);
		//vkCmdDrawIndexed(commandBuffer,
		//	static_cast<uint32_t>(MFloor[0].indices.size()), 1, 0, 0, 0);

		
		MCar.bind(commandBuffer);
		DSCar.bind(commandBuffer, PBlinn, 1, currentImage);
		vkCmdDrawIndexed(commandBuffer,
			static_cast<uint32_t>(MCar.indices.size()), 1, 0, 0, 0);

			PEmission.bind(commandBuffer);
			Msun.bind(commandBuffer);
			DSsun.bind(commandBuffer, PEmission, 0, currentImage);
			DSsunPar.bind(commandBuffer, PEmission, 1, currentImage);
			vkCmdDrawIndexed(commandBuffer,
				static_cast<uint32_t>(Msun.indices.size()), 1, 0, 0, 0);

			Mmoon.bind(commandBuffer);
			DSmoon.bind(commandBuffer, PEmission, 0, currentImage);
			DSmoonPar.bind(commandBuffer, PEmission, 1, currentImage);
			vkCmdDrawIndexed(commandBuffer,
				static_cast<uint32_t>(Mmoon.indices.size()), 1, 0, 0, 0);

			//POPULATE SCENE
			PScene.bind(commandBuffer);
			scene.populateCommandBuffer(commandBuffer, currentImage, PBlinn, DSGlobal);

			PskyBox.bind(commandBuffer);
			MskyBox.bind(commandBuffer);
			DSskyBox.bind(commandBuffer, PskyBox, 0, currentImage);
			DSskyBoxPar.bind(commandBuffer, PskyBox, 1, currentImage);
			vkCmdDrawIndexed(commandBuffer,
				static_cast<uint32_t>(MskyBox.indices.size()), 1, 0, 0, 0);
		}

		txt.populateCommandBuffer(commandBuffer, currentImage, currScene);
		//txt2.populateCommandBuffer(commandBuffer, currentImage, currScene + 1-lightOn);
	}

	void updateUniformBuffer(uint32_t currentImage) {
		//SUN PRAMETERS
		const float turnTime = 72.0f;
		const float angTurnTimeFact = 2.0f * M_PI / turnTime;
		static float cTime = 0.0;
		//CONSTANT MOVEMENT PARAMETERS
		const float ROT_SPEED = glm::radians(120.0f);
		const float MOVE_SPEED = 2.0f;
		
		//CAMERA PARAMETERS
		glm::vec3 CamPos = Pos;
		glm::vec3 CamTarget = Pos;
		static glm::vec3 dampedCamPos = CamPos;
		static float camYaw = glm::radians(45.0f);
    	static float camPitch = glm::radians(30.0f);

		//CONSTANT CAMERA PARAMETERS
		const float fixedCamDist = 10.0f;
		const float lambdaCam = 10.0f;

		//ESC BUTTON ******************************************************************************************** */
		if(glfwGetKey(window, GLFW_KEY_ESCAPE)) {
			glfwSetWindowShouldClose(window, GL_TRUE);
		}

		//CHANGING CAMERA ******************************************************************************************** */		
		if(glfwGetKey(window, GLFW_KEY_P)) {
			if(!debounce) {
				debounce = true;
				curDebounce = GLFW_KEY_P;
				cameraType = (cameraType + 1) % 3;
 
				/*if (cameraType == 0) {
					cameraType = 1;
				}
				else{
					cameraType = 0;
				}*/
			}
		} else {
			if((curDebounce == GLFW_KEY_P) && debounce) {
				debounce = false;
				curDebounce = 0;
			}
		}

		//CHANGING HEADLIGHTS
		if (glfwGetKey(window, GLFW_KEY_1)) {
			if (!debounce) {
				debounce = true;
				curDebounce = GLFW_KEY_1;
				lightOn = 1 - lightOn;
			}
		}
		else {
			if ((curDebounce == GLFW_KEY_1) && debounce) {
				debounce = false;
				curDebounce = 0;
			}
		}
		//BDRF
		if (glfwGetKey(window, GLFW_KEY_2)) {
			if (!debounce) {
				debounce = true;
				curDebounce = GLFW_KEY_2;
				BDRFState = 1 - BDRFState;
			}
		}
		else {
			if ((curDebounce == GLFW_KEY_2) && debounce) {
				debounce = false;
				curDebounce = 0;
			}
		}

		//TURNING OFF CLOUDS
		if (glfwGetKey(window, GLFW_KEY_3)) {
			if (!debounce) {
				debounce = true;
				curDebounce = GLFW_KEY_3;
				showClouds = 1 - showClouds;
			}
		}
		else {
			if ((curDebounce == GLFW_KEY_3) && debounce) {
				debounce = false;
				curDebounce = 0;
			}
		}

		//CHANGING SCENE
		if (glfwGetKey(window, GLFW_KEY_4)) {
			if (!debounce) {
				debounce = true;
				curDebounce = GLFW_KEY_4;
				currScene = (currScene + 1) % 3;
				if (currScene == 0) {
					currScene = 1;
				}
				RebuildPipeline();
			}
		}
		else {
			if ((curDebounce == GLFW_KEY_4) && debounce) {
				debounce = false;
				curDebounce = 0;
			}
		}

		//TIMERS AND MOVEMENT CONTROL ******************************************************************************************** */
		float deltaT;
		glm::vec3 m = glm::vec3(0.0f);
		glm::vec3 r = glm::vec3(0.0f);
		bool fire = false;
		getSixAxis(deltaT, m, r, fire);
		//fisso deltaT per evitare cambi di prestazione su diversi computer/grandezze dis schermi
		//deltaT = 0.02f;
		if (currScene == 2) {
			deltaT = 0;
		}
		

			
		/* CAMERA MOVEMENT ******************************************************************************************** */
		if(cameraType == 0){
			View = LookAt(Pos, r, deltaT);
		}else if(cameraType == 1){
			View = LookInDirection(Pos, r, deltaT);
		}
		else {
			glm::mat4 M = glm::mat4(1.0f / 20.0f, 0, 0, 0,
				0, -4.0f / 60.f, 0, 0,
				0, 0, 1.0f / (-500.0f - 500.0f), 0,
				0, 0, -500.0f / (-500.0f - 500.0f), 1);
			M = M * glm::mat4(1, 0, 0, 0,
				0, cos(glm::radians(35.26f)), sin(glm::radians(35.26f)), 0,
				0, -sin(glm::radians(35.26f)), cos(glm::radians(35.26f)), 0,
				0, 0, 0, 1);
			M = M * glm::mat4(cos(glm::radians(45.0f)), 0, sin(glm::radians(45.0f)), 0,
				0, 1, 0, 0,
				-sin(glm::radians(45.0f)), 0, cos(glm::radians(45.0f)), 0,
				0, 0, 0, 1);
			glm::mat4 Mv = glm::inverse(
				glm::translate(glm::mat4(1), Pos) *
				glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(0, 1, 0)) *
				glm::translate(glm::mat4(1), glm::vec3(0, 2, 8))
			);
			View = M * Mv;
		}

		glm::mat4 M = glm::perspective(glm::radians(45.0f), Ar, 0.1f, 160.0f);
		M[1][1] *= -1;

		glm::mat4 Mv = View;
		
		

		//CAR PARMAT ******************************************************************************************** */
		BlinnMatParUniformBufferObject uboCarMatPar{};
		uboCarMatPar.Power = 200.0;
		uboCarMatPar.isCar = 1.0;
		DSCar.map(currentImage, &uboCarMatPar, 2);
		
		BlinnUniformBufferObject uboCar{};

		/* CAR MOVEMENT ******************************************************************************************** */
		//direzione della macchina
		carDirection = glm::vec3(cos(carYaw), 0, sin(carYaw));

		//angolo di sterzata
		steeringAngle = m.x * ROT_SPEED_CAR * deltaT * m.z;
		
		//raggio di curvatura
		turningRadius = R / tan(steeringAngle);

		//aggiorno la velocità e la limito ad una massima
		Vel += m.z * carDirection * ACC_CAR * deltaT; 
		if(glm::length(Vel) > MAX_SPEED) {
			Vel = glm::normalize(Vel) * MAX_SPEED;
		}

		Pos += Vel * deltaT;

		//aggiorno la posizione se non collide con questo collider
		ColliderQuad col;
		col = updateCollider();

		//VERIFICO COLLISIONE
		if(CollisionQuad(col, colObstacle[0]) || 
		   CollisionQuad(col, colObstacle[1]) || 
		   CollisionQuad(col, colObstacle[2]) || 
		   CollisionQuad(col, colObstacle[3])) 
		{
			Pos -= Vel * deltaT;
			Vel = glm::vec3(0.0f);
		}

		//aggiorno la velocità con l'attrito
		decayFactor = FRICTION * deltaT;
		if (glm::length(Vel) > decayFactor) {
			Vel = glm::normalize(Vel) * (glm::length(Vel) - decayFactor);
		} else {
			Vel = glm::vec3(0.0f);
		}
		
		//aggiorno la posizione e la direzione della macchina
		carYaw -= -m.z * (glm::length(Vel) / turningRadius) * 0.02  ;// * deltaT

		//rotazione iniziale
		initialRotation = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0, -1, 0));
		
		//calcolo la dinamica della macchina
		rotationMatrix = glm::rotate(glm::mat4(1.0f), -carYaw, glm::vec3(0, 1, 0));
		translationMatrix = glm::translate(glm::mat4(1.0f), Pos);

		//calcolo le matrici del modello
		uboCar.mMat = translationMatrix * initialRotation * rotationMatrix;
		uboCar.mvpMat = View * uboCar.mMat;
		uboCar.nMat = glm::transpose(glm::inverse(uboCar.mMat));
		
		DSCar.map(currentImage, &uboCar, 0);
		
		/* BLINN POSITION ******************************************************************************************** */
		BlinnUniformBufferObject blinnUbo{};
		BlinnMatParUniformBufferObject blinnMatParUbo{};


		blinnUbo.mMat = glm::mat4(1);
		blinnUbo.mvpMat = View;
		blinnUbo.nMat = glm::mat4(1);
		DSship.map(currentImage, &blinnUbo, 0);

		blinnMatParUbo.Power = 200.0;
		blinnMatParUbo.Power = 0.0;
		DSship.map(currentImage, &blinnMatParUbo, 2);

		//GLOBAL PARAMETERS ******************************************************************************************** */
		GlobalUniformBufferObject gubo{};

		cTime = cTime + deltaT;
		float forwardOffsetHeadLight = -1.3f;	//offset rispetto al centro dell'auto
		float upwardOffsetHeadLight = 0.75f;	//altezza della camera
		float lateralOffsetHeadLight = 0.25f;
		float lateralOffsetHeadLight2 = -0.25f;


		//SUN
		gubo.lightDir[0] = glm::vec3(cos(cTime * angTurnTimeFact), sin(cTime * angTurnTimeFact), 0);
		glm::vec4 sunColor;
		sunColor = glm::vec4(0.5 + 0.5 * sin(cTime * angTurnTimeFact), sin(cTime * angTurnTimeFact), sin(cTime * angTurnTimeFact), 1.0);
		if (sunColor.y < 0) {
			sunColor = glm::vec4(0.0, 0.0, 0.0, 1.0);
		}
		gubo.lightColor[0] = sunColor;

		gubo.eyePos = glm::vec3(glm::inverse(View) * glm::vec4(0, 0, 0, 1));

		//HEADLIGHTS
		gubo.lightDir[1] = carDirection;
		gubo.lightColor[1] = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		gubo.lightPos[1] = Pos + carDirection * forwardOffsetHeadLight
			+ glm::vec3(0, upwardOffsetHeadLight, 0)
			+ glm::normalize(glm::cross(carDirection, glm::vec3(0, 1, 0))) * lateralOffsetHeadLight;
		gubo.lightDir[2] = carDirection;
		gubo.lightColor[2] = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		gubo.lightPos[2] = Pos + carDirection * forwardOffsetHeadLight
			+ glm::vec3(0, upwardOffsetHeadLight, 0)
			+ glm::normalize(glm::cross(carDirection, glm::vec3(0, 1, 0))) * lateralOffsetHeadLight2;

		//MOON
		gubo.lightDir[3] = glm::vec3(-cos(cTime * angTurnTimeFact), -sin(cTime * angTurnTimeFact), 0);
		glm::vec4 moonColor;
		moonColor = glm::vec4(0.0, 0.0, -sin(cTime * angTurnTimeFact) * 0.8 , 1.0);
		if (moonColor.z < 0) {
			moonColor = glm::vec4(0.0, 0.0, 0.0, 1.0);
		}
		gubo.lightColor[3] = moonColor;

		gubo.cosIn = 0.96;
		gubo.cosOut = 0.86;
		gubo.lightOn = lightOn;
		gubo.BDRFState = BDRFState;

		DSGlobal.map(currentImage, &gubo, 0);

		//SUNPARAMETERS

		EmissionUniformBufferObject emissionUbo{};
		emissionUbo.mvpMat = View * translate(glm::mat4(1), gubo.lightDir[0] * 500.0f) * glm::scale(glm::mat4(1), glm::vec3(20, 20, 20)) * baseTr;
		DSsun.map(currentImage, &emissionUbo, 0);

		EmissionUniformBufferObject moonUbo{};
		moonUbo.mvpMat = View * glm::translate(glm::mat4(1), gubo.lightDir[3] * 500.0f) * glm::scale(glm::mat4(1), glm::vec3(20, 20, 20)) * baseTr;
		DSmoon.map(currentImage, &moonUbo, 0);

		SunParUniformBufferObject sunParUbo{};
		sunParUbo.sinSun = sin(cTime * angTurnTimeFact);
		sunParUbo.isMoon = 1.0;
		DSsunPar.map(currentImage, &sunParUbo, 0);

		SunParUniformBufferObject moonParUbo{};
		moonParUbo.sinSun = sin(cTime * angTurnTimeFact);
		moonParUbo.isMoon = 0.0;
		DSmoonPar.map(currentImage, &moonParUbo, 0);

		/* SCENE POSITION ******************************************************************************************** */
		int i = 0;
		for(const auto& instance : scene.instancesParsed) {
            BlinnUniformBufferObject uboScene{};
			BlinnMatParUniformBufferObject uboSceneMatPar{};

			uboSceneMatPar.Power = 200.0;
			uboSceneMatPar.isCar = 0.0;

			uboScene.mMat = instance.second.transform;
			uboScene.mvpMat = View * uboScene.mMat;
			uboScene.nMat = glm::transpose(glm::inverse(uboScene.mMat));

			scene.DSScene[i]->map(currentImage, &uboScene, 0);
			scene.DSScene[i]->map(currentImage, &uboSceneMatPar, 2);

			i++;
        }

		/* FLOOR POSITION ******************************************************************************************** */
		/*
		BlinnUniformBufferObject uboFloor{};
		BlinnMatParUniformBufferObject uboFloorMatPar{};

		uboFloor.mMat = glm::mat4( 54, 0, 0, 0,
								   0, 54, 0, 0,
								   0, 0, 54, 0,
								   -15, -53.99, 30, 1);
		uboFloor.mvpMat = View * uboFloor.mMat;
		uboFloor.nMat = glm::transpose(glm::inverse(uboFloor.mMat));

		uboFloorMatPar.Power = 200.0;

		DSFloor.map(currentImage, &uboFloor, 0);
		DSFloor.map(currentImage, &uboFloorMatPar, 2);
		*/
		//SKYBOX

		skyBoxUniformBufferObject sbubo{};
		sbubo.mvpMat = M * glm::mat4(glm::mat3(Mv));
		DSskyBox.map(currentImage, &sbubo, 0);

		SkyMatParUniformBufferObject sbparubo{};
		sbparubo.Ang = cTime * angTurnTimeFact/10;
		sbparubo.ShowCloud = showClouds;
		sbparubo.sinSun = sin(cTime * angTurnTimeFact);
		DSskyBoxPar.map(currentImage, &sbparubo, 0);
    
		
	}	

	//MOVE THE CAR
	glm::mat4 MoveCar(glm::vec3 Pos, float Yaw) {
		glm::mat4 M = glm::mat4(1.0f);

		glm::mat4 MT = glm::translate(glm::mat4(1.0f), glm::vec3(Pos[0], 0, Pos[2]));
		glm::mat4 MYaw = glm::rotate(glm::mat4(1.0f), Yaw, glm::vec3(0, 1, 0));
		glm::mat4 MS = glm::scale(glm::mat4(1.0f), glm::vec3(1, 1, 1));
		M = MT * MYaw * MS;

		return MT;
	}

	//LOOK AT PROJECTION MATRIX
	glm::mat4 MakeViewProjectionLookAt(glm::vec3 Pos, glm::vec3 Target, glm::vec3 Up, float Roll, float FOVy, float Ar, float nearPlane, float farPlane) {

		glm::mat4 M = glm::mat4(1.0f);
		M = glm::perspective(FOVy, Ar, nearPlane, farPlane);

		glm::mat4 Mv = glm::rotate(glm::mat4(1.0), -Roll, glm::vec3(0, 0, 1)) * glm::scale(glm::mat4(1.0), glm::vec3(1,-1,1))* glm::lookAt(Pos, Target, glm::vec3(Up[0], Up[1], Up[2]));
		M = M * Mv;

		return M;
	}

	//LOOK IN DIRECTION MATRIX
	glm::mat4 MakeViewProjectionLookInDirection(glm::vec3 Pos, float Yaw, float Pitch, float Roll, float FOVy, float Ar, float nearPlane, float farPlane) {

		glm::mat4 M = glm::mat4(1.0f);

		M = glm::perspective(FOVy, Ar, nearPlane, farPlane);

		glm::mat4 MT = glm::translate(glm::mat4(1.0f), glm::vec3(-Pos[0], -Pos[1], -Pos[2]));
		glm::mat4 MYaw = glm::rotate(glm::mat4(1.0f), Yaw, glm::vec3(0, -1, 0));
		glm::mat4 MPitch = glm::rotate(glm::mat4(1.0f), Pitch, glm::vec3(-1, 0, 0));
		glm::mat4 MRoll = glm::rotate(glm::mat4(1.0f), Roll, glm::vec3(0, 0, -1));
		glm::mat4 MS = glm::scale(glm::mat4(1.0), glm::vec3(1,-1,1));

		M = M * MS * MRoll * MPitch * MYaw * MT;

		return M;
	}

	//LOOK IN DIRECTION MATRIX
	glm::mat4 MakeIsometricViewProjectionLookAt(glm::vec3 Pos, float Yaw, float Pitch, float Roll, float FOVy, float Ar, float nearPlane, float farPlane) {

		glm::mat4 M = glm::mat4(1.0f);

		M = glm::perspective(FOVy, Ar, nearPlane, farPlane);

		glm::mat4 MT = glm::translate(glm::mat4(1.0f), glm::vec3(-Pos[0], -Pos[1], -Pos[2]));
		glm::mat4 MYaw = glm::rotate(glm::mat4(1.0f), Yaw, glm::vec3(0, -1, 0));
		glm::mat4 MPitch = glm::rotate(glm::mat4(1.0f), Pitch, glm::vec3(-1, 0, 0));
		glm::mat4 MRoll = glm::rotate(glm::mat4(1.0f), Roll, glm::vec3(0, 0, -1));
		glm::mat4 MS = glm::scale(glm::mat4(1.0), glm::vec3(1, -1, 1));

		M = M * MS * MRoll * MPitch * MYaw * MT;

		return M;
	}

	//CAMERA LOOK AT FUNCTION
	glm::mat4 LookAt(glm::vec3 Pos, glm::vec3 r, float deltaT) {

		glm::vec3 CamPos = Pos;
		glm::vec3 CamTarget = Pos;
		static glm::vec3 dampedCamPos = CamPos;
		static float camYaw = glm::radians(45.0f);
    	static float camPitch = glm::radians(30.0f);

		//Calcolo Yaw e Pitch alla camera
		camYaw += r.y * ROT_SPEED_CAMERA * deltaT;
		camPitch += r.x * ROT_SPEED_CAMERA * deltaT;
		camPitch = glm::clamp(camPitch, glm::radians(-89.0f), glm::radians(89.0f)); //evito che gli assi si sovrappongano

		//Calcolo la posizione della camera (deve sempre putare al target)
		CamPos = CamTarget + glm::vec3(glm::rotate(glm::mat4(1), camYaw, glm::vec3(0,1,0)) * 
                                   glm::rotate(glm::mat4(1), -camPitch, glm::vec3(1,0,0)) * 
                                   glm::vec4(0,0,fixedCamDist,1));
		
		//Rendo il movimento della camera più fluido con un esponenziale
		dampedCamPos = CamPos * (1 - exp(-lambdaCam * deltaT)) + dampedCamPos * exp(-lambdaCam * deltaT); 
		
		//Creo la matrice di view e projection
		return MakeViewProjectionLookAt(dampedCamPos, CamTarget, glm::vec3(0,1,0), 0.0f, FOVy, Ar, nearPlane, farPlane);
	}

	//CAMERA IN DIRECTION FUNCTION
	glm::mat4 LookInDirection(glm::vec3 Pos, glm::vec3 r, float deltaT) {

		glm::vec3 CamPos = Pos;

		float forwardOffset = -0.45f;	//offset rispetto al centro dell'auto
		float upwardOffset = 0.75f;	//altezza della camera
		float lateralOffset = 0.25f; 
		
		//Calcolo la posizione della camera
		glm::vec3 CamTarget = Pos + carDirection * forwardOffset 
								  + glm::vec3(0, upwardOffset, 0) 
								  + glm::normalize(glm::cross(carDirection, glm::vec3(0, 1, 0))) * lateralOffset;

		static float camYaw = glm::radians(45.0f);
    	static float camPitch = glm::radians(30.0f);

		//Calcolo Yaw e Pitch alla camera
		camYaw += r.y * ROT_SPEED_CAMERA * deltaT;
		camPitch += r.x * ROT_SPEED_CAMERA * deltaT;

		camPitch = glm::clamp(camPitch, glm::radians(-89.0f), glm::radians(89.0f)); //evito che gli assi si sovrappongano

		//Calcolo la posizione della camera
		CamPos = CamTarget + glm::vec3(glm::rotate(glm::mat4(1), camYaw, glm::vec3(0,1,0)) * 
                                   glm::rotate(glm::mat4(1), -camPitch, glm::vec3(1,0,0)) * 
                                   glm::vec4(0,0,PilotDist,1)); 
		
		//Creo la matrice di view e projection
		return MakeViewProjectionLookInDirection(CamPos, camYaw - carYaw, camPitch, 0.0f, FOVy, Ar, nearPlane, farPlane); //using carYaw with camYaw to maintain the relative angle of the camera
	}
	
	//CREATE THE FLOOR
	void MakeFloor(float size, std::vector<std::array<float,6>> &vertices, std::vector<uint32_t> &indices) {

		vertices = {
					{-size / 2.0f, size / 2.0f,-size / 2.0f, 0, 1, 0},
					{-size / 2.0f, size / 2.0f, size / 2.0f, 0, 1, 0},
					{ size / 2.0f, size / 2.0f,-size / 2.0f, 0, 1, 0},
					{ size / 2.0f, size / 2.0f, size / 2.0f, 0, 1, 0},

					{-size / 2.0f, -size / 2.0f,-size / 2.0f, 0, -1, 0},
					{-size / 2.0f, -size / 2.0f, size / 2.0f, 0, -1, 0},
					{ size / 2.0f, -size / 2.0f,-size / 2.0f, 0, -1, 0},
					{ size / 2.0f, -size / 2.0f, size / 2.0f, 0, -1, 0},

					{-size / 2.0f, size / 2.0f,-size / 2.0f, -1, 0, 0},
					{-size / 2.0f, size / 2.0f,-size / 2.0f, 0, 0, -1},

					{-size / 2.0f, size / 2.0f, size / 2.0f, -1, 0, 0},
					{-size / 2.0f, size / 2.0f, size / 2.0f, 0, 0, 1},

					{ size / 2.0f, size / 2.0f,-size / 2.0f, 1, 0, 0},
					{ size / 2.0f, size / 2.0f,-size / 2.0f, 0, 0, -1},

					{ size / 2.0f, size / 2.0f, size / 2.0f, 1, 0, 0},
					{ size / 2.0f, size / 2.0f, size / 2.0f, 0, 0, 1},

					{-size / 2.0f, -size / 2.0f,-size / 2.0f, -1, 0, 0},
					{-size / 2.0f, -size / 2.0f,-size / 2.0f, 0, 0, -1},

					{-size / 2.0f, -size / 2.0f, size / 2.0f, -1, 0, 0},
					{-size / 2.0f, -size / 2.0f, size / 2.0f, 0, 0, 1},

					{ size / 2.0f, -size / 2.0f,-size / 2.0f, 1, 0, 0},
					{ size / 2.0f, -size / 2.0f,-size / 2.0f, 0, 0, -1},

					{ size / 2.0f, -size / 2.0f, size / 2.0f, 1, 0, 0},
					{ size / 2.0f, -size / 2.0f, size / 2.0f, 0, 0, 1},
		};

		indices = {
					0, 1, 2, 1, 3, 2,
					22, 12, 14, 22, 20, 12,
					17, 13, 21, 9, 13, 17,
					18, 8, 16, 10, 8, 18,
					11, 19, 15, 15, 19, 23,
					5, 6, 7, 4, 6, 5 
		};
	}

	//COLLISION DETECTION
	bool CollisionQuad(ColliderQuad obj1, ColliderQuad obj2){
		
		if(CollisionQuadPoint(obj1.vertice1, obj2) || CollisionQuadPoint(obj1.vertice2, obj2) || CollisionQuadPoint(obj1.vertice3, obj2) || CollisionQuadPoint(obj1.vertice4, obj2)){
			return true;
		if(CollisionQuadPoint(obj2.vertice1, obj1) || CollisionQuadPoint(obj2.vertice2, obj1) || CollisionQuadPoint(obj2.vertice3, obj1) || CollisionQuadPoint(obj2.vertice4, obj1)){
			return true;
		}
		}else{
			return false;
		}
	}
	bool CollisionQuadPoint(glm::vec3 point, ColliderQuad obj){
		//determining which point has the highest x and z coordinates
		float maxX = obj.vertice1.x;
		float minX = obj.vertice1.x;

		float maxZ = obj.vertice1.z;
		float minZ = obj.vertice1.z;

		if(obj.vertice2.x > maxX){
			maxX = obj.vertice2.x;
		}else {
			minX = obj.vertice2.x;

			if(obj.vertice3.x >= maxX){
				maxX = obj.vertice3.x;
			} else{
				minX = obj.vertice3.x;
			}
		}

		if(obj.vertice2.z > maxZ){
			maxZ = obj.vertice2.z;
		}else {
			minZ = obj.vertice2.z;

			if(obj.vertice3.z >= maxZ){
				maxZ = obj.vertice3.z;
			} else{
				minZ = obj.vertice3.z;
			}
		}

		//checking if the point is inside the quad
		if(point.x >= minX && point.x <= maxX && point.z >= minZ && point.z <= maxZ){
			return true;
		}else{
			return false;
		}
	}
	void InitializeColliders(){
		
		colObstacle[0].vertice1 = glm::vec3(-5, 0, 7);
		colObstacle[0].vertice2 = glm::vec3(-5, 0, -7);
		colObstacle[0].vertice3 = glm::vec3(5, 0, 7);
		colObstacle[0].vertice4 = glm::vec3(5, 0, -7);

		colObstacle[1].vertice1 = glm::vec3(22, 0, 10);
		colObstacle[1].vertice2 = glm::vec3(22, 0, 20);
		colObstacle[1].vertice3 = glm::vec3(8, 0, 10);
		colObstacle[1].vertice4 = glm::vec3(8, 0, 20);

		colObstacle[2].vertice1 = glm::vec3(-25, 0, 38);
		colObstacle[2].vertice2 = glm::vec3(-25, 0, 52);
		colObstacle[2].vertice3 = glm::vec3(-35, 0, 38);
		colObstacle[2].vertice4 = glm::vec3(-35, 0, 52);

		colObstacle[3].vertice1 = glm::vec3(-38, 0, 55);
		colObstacle[3].vertice2 = glm::vec3(-38, 0, 65);
		colObstacle[3].vertice3 = glm::vec3(-52, 0, 55);
		colObstacle[3].vertice4 = glm::vec3(-52, 0, 65);


	}
	ColliderQuad updateCollider(){
		ColliderQuad col;

		glm::vec3 halfExtents = glm::vec3(R / 2, 0, W / 2);
		glm::vec3 localVertice1 = glm::vec3(-halfExtents.x, 0, -halfExtents.z);
		glm::vec3 localVertice2 = glm::vec3(halfExtents.x, 0, -halfExtents.z);
		glm::vec3 localVertice3 = glm::vec3(halfExtents.x, 0, halfExtents.z);
		glm::vec3 localVertice4 = glm::vec3(-halfExtents.x, 0, halfExtents.z);
		
		col.vertice1 = Pos + glm::vec3(rotationMatrix * glm::vec4(localVertice1, 1.0));
		col.vertice2 = Pos + glm::vec3(rotationMatrix * glm::vec4(localVertice2, 1.0));
		col.vertice3 = Pos + glm::vec3(rotationMatrix * glm::vec4(localVertice3, 1.0));
		col.vertice4 = Pos + glm::vec3(rotationMatrix * glm::vec4(localVertice4, 1.0));
		return col;
	}

};



	


// This is the main: probably you do not need to touch this!
int main() {
    MeshLoader app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}