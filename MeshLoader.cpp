// This has been adapted from the Vulkan tutorial
#define GLM_ENABLE_EXPERIMENTAL
#define NSHIP 16
#include "Starter.hpp"
#include "modules/Scene.hpp"


// The uniform buffer objects data structures
// Remember to use the correct alignas(...) value
//        float : alignas(4)
//        vec2  : alignas(8)
//        vec3  : alignas(16)
//        vec4  : alignas(16)
//        mat3  : alignas(16)
//        mat4  : alignas(16)

/********************Uniform Blocks********************/
//UNIFORM BUFFER - BLINN -
#define NLIGHTS 4
struct BlinnUniformBufferObject {
	alignas(16) glm::mat4 mvpMat;
	alignas(16) glm::mat4 mMat;
	alignas(16) glm::mat4 nMat;
};

struct BlinnMatParUniformBufferObject {
	alignas(4)  float Power;
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

//HEADLIGHTS

struct SpotUniformBufferObject {
	alignas(16) glm::vec3 lightDir;
	alignas(16) glm::vec3 lightPos[2];
	alignas(16) glm::vec4 lightColor;
	alignas(4) float cosIn;
	alignas(4) float cosOut;
	alignas(16) glm::vec3 eyePos;
	alignas(16) glm::vec4 eyeDir;
	alignas(4) float lightOn;
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
	//SYSTEM PARAMETERS
		bool debounce = false;	//avoid multiple key press
		int curDebounce = 0;
		SceneProject scene;

	//CAMERA PARAMETERS
		float Ar;	// Current aspect ratio
		int cameraType = 0;	//0 = camera look at, 1 = camera look in direction
		
		//CONSTANTS
		const float FOVy = glm::radians(90.0f);
		const float nearPlane = 0.1f;
		const float farPlane = 500.0f;
		
		const float ROT_SPEED_CAMERA = glm::radians(120.0f);
		const float fixedCamDist = 10.0f;	//distanza fissa del target
		const float PilotDist = 0.0f;	//distanza del pilota dal target
		const float lambdaCam = 10.0f;		//costante per l'esponenziale della camera

	//CAR PARAMETERS
		float carYaw = 0.0f;
		float decayFactor = 0.0f;
		float turningRadius = 0.0f;
		float steeringAngle = 0.0f;

		glm::vec3 carDirection;
		glm::vec3 Pos = glm::vec3(0.0f, 0.0f, 0.0f);
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
			
	//MATRICES
		glm::mat4 View;
		glm::mat4 baseTr = glm::translate(glm::mat4(1), glm::vec3(0, 0, 0));

	//********************DESCRIPTOR SET LAYOUT ["classes" of what will be passed to the shaders]
	DescriptorSetLayout DSLGlobal;
	DescriptorSetLayout DSLBlinn;	// For Blinn Objects
	DescriptorSetLayout DSLEmission;
	DescriptorSetLayout DSLScene;
	//DescriptorSetLayout DSLSpot;

	//********************VERTEX DESCRIPTOR
	VertexDescriptor VDBlinn;
	VertexDescriptor VDEmission;
	VertexDescriptor VDScene;

	//********************PIPELINES [Shader couples]
	Pipeline PBlinn;
	Pipeline PEmission;
	Pipeline PScene;

	// Models, textures and Descriptors (values assigned to the uniforms)
	// Please note that Model objects depends on the corresponding vertex structure
	//********************MODELS
	Model MCar;
	Model Mship;

	Model Msun;
	Texture Tsun;
	DescriptorSet DSsun;

	Model Mmoon;
	Texture Tmoon;
	DescriptorSet DSmoon;


	//********************DESCRIPTOR SETS
	DescriptorSet DSCar;
	DescriptorSet DSGlobal;
	DescriptorSet DSship;
	//DescriptorSet DSSpot;

	//********************TEXTURES
	Texture TCar;
	Texture Tship;

	void setWindowParameters() {
		// window size, titile and initial background
		windowWidth = 800;
		windowHeight = 600;
		windowTitle = "Mesh Loader";
    	windowResizable = GLFW_TRUE;
		initialBackgroundColor = {0.0f, 0.005f, 0.01f, 1.0f};
		
		// Descriptor pool sizes
		DPSZs.uniformBlocksInPool =11;//aumento di 2
		DPSZs.texturesInPool = 8;//aumentato di 1
		DPSZs.setsInPool = 8;//aumento di 1
		
		Ar = (float)windowWidth / (float)windowHeight;
	}
	
	// What to do when the window changes size
	void onWindowResize(int w, int h) {
		Ar = (float)w / (float)h;
	}
	
	void localInit() {

		lightOn = 1.0;
		BDRFState = 1.0;
		// Descriptor Layouts INITIALIZATION [what will be passed to the shaders]
		DSLGlobal.init(this, {
			{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(GlobalUniformBufferObject), 1}
		});
		DSLEmission.init(this, {
					{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(EmissionUniformBufferObject), 1},
					{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1}
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
		// The second parameter is the pointer to the vertex definition
		// Third and fourth parameters are respectively the vertex and fragment shaders
		// The last array, is a vector of pointer to the layouts of the sets that will
		// be used in this pipeline. The first element will be set 0, and so on..
		PEmission.init(this, &VDEmission, "shaders/EmissionVert.spv", "shaders/EmissionFrag.spv", { &DSLEmission });
		PBlinn.init(this, &VDBlinn,  "shaders/CarVert.spv",    "shaders/CarFrag.spv", {&DSLGlobal, &DSLBlinn/*,&DSLSpot*/});
		PScene.init(this, &VDScene, "shaders/CarVert.spv", "shaders/CarFrag.spv", {&DSLGlobal, &DSLScene});

		// Create models		
		MCar.init(this, &VDBlinn, "Models/Car.mgcg", MGCG);
		Mship.init(this, &VDBlinn, "models/X-WING-baker.obj", OBJ);
		Msun.init(this, &VDEmission, "models/Sphere.obj", OBJ);
		Mmoon.init(this, &VDEmission, "models/Sphere.obj", OBJ);

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
		
		//INITIALIZE THE SCENE
		scene.init(this, &VDScene, DSLScene, PScene, "modules/scene.json");

	}
	
	void pipelinesAndDescriptorSetsInit() {
		// This creates a new pipeline (with the current surface), using its shaders
		PBlinn.create();
		PEmission.create();
		PScene.create();

		// Here you define the data set
		DSsun.init(this, &DSLEmission, { &Tsun });
		DSmoon.init(this, &DSLEmission, { &Tmoon });
		DSGlobal.init(this, &DSLGlobal, {});
		DSship.init(this, &DSLBlinn, {&Tship});
		DSCar.init(this, &DSLBlinn, {&TCar});
		
		scene.pipelinesAndDescriptorSetsInit(DSLScene);
		
	}

	void pipelinesAndDescriptorSetsCleanup() {
		// Cleanup pipelines
		PBlinn.cleanup();
		PEmission.cleanup();
		PScene.cleanup();

		DSGlobal.cleanup();
		DSCar.cleanup();
		DSship.cleanup();
		DSsun.cleanup();
		DSmoon.cleanup();
		
		//SCENE CLEANUP
		scene.pipelinesAndDescriptorSetsCleanup();
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
		
		// Cleanup descriptor set layouts
		DSLGlobal.cleanup();
		DSLBlinn.cleanup();
		DSLEmission.cleanup();
		DSLScene.cleanup();
		//DSLSpot.cleanup();
		
		// Destroies the pipelines
		PBlinn.destroy();
		PEmission.destroy();
		PScene.destroy();

		//SCENE CLEANUP
		scene.localCleanup();
	}
	
	void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage) {
		
		PBlinn.bind(commandBuffer);
		Mship.bind(commandBuffer);
		DSGlobal.bind(commandBuffer, PBlinn, 0, currentImage);	// The Global Descriptor Set (Set 0)
		DSship.bind(commandBuffer, PBlinn, 1, currentImage);
		vkCmdDrawIndexed(commandBuffer,
						static_cast<uint32_t>(Mship.indices.size()), 1, 0, 0, 0);	
		


		MCar.bind(commandBuffer);
		DSCar.bind(commandBuffer, PBlinn, 1, currentImage);
		vkCmdDrawIndexed(commandBuffer,
				static_cast<uint32_t>(MCar.indices.size()), 1, 0, 0, 0);

		PEmission.bind(commandBuffer);
		Msun.bind(commandBuffer);
		DSsun.bind(commandBuffer, PEmission, 0, currentImage);
		vkCmdDrawIndexed(commandBuffer,
			static_cast<uint32_t>(Msun.indices.size()), 1, 0, 0, 0);

		//POPULATE SCENE
		PScene.bind(commandBuffer);
		scene.populateCommandBuffer(commandBuffer, currentImage, PScene, DSGlobal);

		Mmoon.bind(commandBuffer);
		DSmoon.bind(commandBuffer, PEmission, 0, currentImage);
		vkCmdDrawIndexed(commandBuffer,
			static_cast<uint32_t>(Mmoon.indices.size()), 1, 0, 0, 0);
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
				curDebounce = GLFW_KEY_SPACE;
				
				if(cameraType == 0){
					cameraType = 1;
				}
				else{
					cameraType = 0;
				}
			}
		} else {
			if((curDebounce == GLFW_KEY_SPACE) && debounce) {
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

		//TIMERS AND MOVEMENT CONTROL ******************************************************************************************** */
		float deltaT;
		glm::vec3 m = glm::vec3(0.0f);
		glm::vec3 r = glm::vec3(0.0f);
		bool fire = false;
		getSixAxis(deltaT, m, r, fire);
		
		/* CAMERA MOVEMENT ******************************************************************************************** */
		if(cameraType == 0){
			View = LookAt(Pos, r, deltaT);
		}else{
			View = LookInDirection(Pos, r, deltaT);
		}
		
		

		//CAR PARMAT ******************************************************************************************** */
		BlinnMatParUniformBufferObject uboCarMatPar{};
		uboCarMatPar.Power = 200.0;
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

		//aggiorno la posizione
		Pos += Vel * deltaT;

		//aggiorno la velocità con l'attrito
		decayFactor = FRICTION * deltaT;
		if (glm::length(Vel) > decayFactor) {
			Vel = glm::normalize(Vel) * (glm::length(Vel) - decayFactor);
		} else {
			Vel = glm::vec3(0.0f);
		}
		
		//aggiorno la posizione e la direzione della macchina
		carYaw -= -m.z * (glm::length(Vel) / turningRadius) * deltaT;

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
		gubo.lightDir[3] = glm::vec3(cos(cTime * angTurnTimeFact), sin(cTime * angTurnTimeFact), 0);
		glm::vec4 moonColor;
		moonColor = glm::vec4(0.5*(1 + sin(cTime * angTurnTimeFact)), 0.0, -sin(cTime * angTurnTimeFact), 1.0);
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
		emissionUbo.mvpMat = View * glm::translate(glm::mat4(1), gubo.lightDir[0] * 40.0f) * baseTr;
		DSsun.map(currentImage, &emissionUbo, 0);

		EmissionUniformBufferObject moonUbo{};
		moonUbo.mvpMat = View * glm::translate(glm::mat4(1), gubo.lightDir[3] * -40.0f) * baseTr;
		DSmoon.map(currentImage, &moonUbo, 0);

		/* SCENE POSITION ******************************************************************************************** */
        int i = 0;
		for(const auto& instance : scene.instancesParsed) {
            BlinnUniformBufferObject uboScene{};
			BlinnMatParUniformBufferObject uboSceneMatPar{};

			uboSceneMatPar.Power = 200.0;

			uboScene.mMat = instance.second.transform;
			uboScene.mvpMat = View * uboScene.mMat;
			uboScene.nMat = glm::transpose(glm::inverse(uboCar.mMat));

			scene.DSScene[i].map(currentImage, &uboScene, 0);
			scene.DSScene[i].map(currentImage, &uboSceneMatPar, 2);

			i++;
        }
    
		
/*
		//SPOT PARAMETERS ********************************************************************************************

		// updates global uniforms
		SpotUniformBufferObject subo{};

		//		gubo.lightDir = glm::vec3(cos(glm::radians(135.0f)), sin(glm::radians(135.0f)), 0.0f);
		for (int i = 0; i < 2; i++) {
			subo.lightColor = glm::vec4(1);
			subo.lightDir = carDirection;
			subo.lightPos[i] = glm::vec3(0, 1, 0);
		}
		subo.cosIn = 0.3;
		subo.cosOut = 0.4;

		subo.eyePos = CamPos;
		subo.lightOn = 1;
		*/
	}	

	//Move the car, todo: implement dynamic
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