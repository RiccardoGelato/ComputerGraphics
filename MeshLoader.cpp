// This has been adapted from the Vulkan tutorial
#define GLM_ENABLE_EXPERIMENTAL
#include "Starter.hpp"

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
#define NSHIP 16
struct BlinnUniformBufferObject {
	alignas(16) glm::mat4 mvpMat;
	alignas(16) glm::mat4 mMat;
	alignas(16) glm::mat4 nMat;
};

struct BlinnMatParUniformBufferObject {
	alignas(4)  float Power;
};

//UNIFORM BUFFER - CAR -
struct CarUniformBufferObject {
	alignas(16) glm::mat4 mvpMat;
	alignas(16) glm::mat4 mMat;
	alignas(16) glm::mat4 nMat;
};

struct CarMatParUniformBufferObject {
	alignas(4)  float Power;
};

//UNIFORM BUFFER - GLOBAL -
struct GlobalUniformBufferObject {
	alignas(16) glm::vec3 lightDir;
	alignas(16) glm::vec4 lightColor;
	alignas(16) glm::vec3 eyePos;
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
//VERTICES - CAR -
struct CarVertex {
	glm::vec3 pos;
	glm::vec3 norm;
	glm::vec2 UV;
};

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

	//MATRICES
		glm::mat4 View;
		glm::mat4 baseTr = glm::translate(glm::mat4(1), glm::vec3(0, 0, 0));

	//********************DESCRIPTOR SET LAYOUT ["classes" of what will be passed to the shaders]
	DescriptorSetLayout DSLCar;
	DescriptorSetLayout DSLGlobal;
	DescriptorSetLayout DSLBlinn;	// For Blinn Objects
	DescriptorSetLayout DSLEmission;
	//DescriptorSetLayout DSLSpot;

	//********************VERTEX DESCRIPTOR
	VertexDescriptor VDCar;
	VertexDescriptor VDBlinn;
	VertexDescriptor VDEmission;

	//********************PIPELINES [Shader couples]
	Pipeline PCar;
	Pipeline PBlinn;
	Pipeline PEmission;

	// Models, textures and Descriptors (values assigned to the uniforms)
	// Please note that Model objects depends on the corresponding vertex structure
	//********************MODELS
	Model MCar;
	Model Mship;

	Model Msun;
	Texture Tsun;
	DescriptorSet DSsun;


	//********************DESCRIPTOR SETS
	DescriptorSet DSCar;
	DescriptorSet DSGlobal;
	DescriptorSet DSship;
	//DescriptorSet DSSpot;

	//********************TEXTURES
	Texture TCar;
	Texture Tship;
	
	
	// C++ storage for uniform variables

	// Other application parameters

	// Here you set the main application parameters
	void setWindowParameters() {
		// window size, titile and initial background
		windowWidth = 800;
		windowHeight = 600;
		windowTitle = "Mesh Loader";
    	windowResizable = GLFW_TRUE;
		initialBackgroundColor = {0.0f, 0.005f, 0.01f, 1.0f};
		
		// Descriptor pool sizes
		DPSZs.uniformBlocksInPool = 7;
		DPSZs.texturesInPool = 6;
		DPSZs.setsInPool = 5;
		
		Ar = (float)windowWidth / (float)windowHeight;
	}
	
	// What to do when the window changes size
	void onWindowResize(int w, int h) {
		Ar = (float)w / (float)h;
	}
	
	// Here you load and setup all your Vulkan Models and Texutures.
	// Here you also create your Descriptor set layouts and load the shaders for the pipelines
	void localInit() {
		// Descriptor Layouts INITIALIZATION [what will be passed to the shaders]

		DSLGlobal.init(this, {
					{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(GlobalUniformBufferObject), 1}
				});
		DSLEmission.init(this, {
					{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(EmissionUniformBufferObject), 1},
					{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1}
			});

		DSLCar.init(this, {
					{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(CarUniformBufferObject), 1},
					{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1},
					{2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(CarMatParUniformBufferObject), 1}
				});
		DSLBlinn.init(this, {
			{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(BlinnUniformBufferObject), 1},
			{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1},
			{2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(BlinnMatParUniformBufferObject), 1}
		});
		/*DSLSpot.init(this, {
					{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(SpotUniformBufferObject)}
			});*/

		// Vertex descriptors INITIALIZATION
		VDCar.init(this, {
				  {0, sizeof(CarVertex), VK_VERTEX_INPUT_RATE_VERTEX}
				}, {
				  {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(CarVertex, pos),
				         sizeof(glm::vec3), POSITION},
				  {0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(CarVertex, norm),
				         sizeof(glm::vec3), NORMAL},
				  {0, 2, VK_FORMAT_R32G32_SFLOAT, offsetof(CarVertex, UV),
				         sizeof(glm::vec2), UV}
				});
		
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
		PCar.init(this, &VDCar, "shaders/CarVert.spv", "shaders/CarFrag.spv", {&DSLGlobal, &DSLCar/*, &DSLSpot*/});
		PEmission.init(this, &VDEmission, "shaders/EmissionVert.spv", "shaders/EmissionFrag.spv", { &DSLEmission });

		PBlinn.init(this, &VDBlinn,  "shaders/CarVert.spv",    "shaders/CarFrag.spv", {&DSLGlobal, &DSLBlinn/*,&DSLSpot*/});

		// Models, textures and Descriptors (values assigned to the uniforms)

		// Create models
		// The second parameter is the pointer to the vertex definition for this model
		// The third parameter is the file name
		// The last is a constant specifying the file type: currently only OBJ or GLTF
		
		MCar.init(this, &VDCar, "Models/Car.mgcg", MGCG);
		Mship.init(this, &VDBlinn, "models/X-WING-baker.obj", OBJ);
		Msun.init(this, &VDEmission, "models/Sphere.obj", OBJ);

		/*// Creates a mesh with direct enumeration of vertices and indices
		M4.vertices = {{{-6,-2,-6}, {0.0f,0.0f}}, {{-6,-2,6}, {0.0f,1.0f}},
					    {{6,-2,-6}, {1.0f,0.0f}}, {{ 6,-2,6}, {1.0f,1.0f}}};
		M4.indices = {0, 1, 2,    1, 3, 2};
		M4.initMesh(this, &VD);*/

		// Create the textures
		// The second parameter is the file name
		TCar.init(this, "textures/CarTexture.png");
		Tship.init(this, "textures/XwingColors.png");
		Tsun.init(this, "textures/2k_sun.jpg");

	}
	
	// Here you create your pipelines and Descriptor Sets!
	void pipelinesAndDescriptorSetsInit() {
		// This creates a new pipeline (with the current surface), using its shaders
		PCar.create();
		PBlinn.create();
		PEmission.create();

		// Here you define the data set
		DSsun.init(this, &DSLEmission, { &Tsun });
		DSGlobal.init(this, &DSLGlobal, {});
		DSship.init(this, &DSLBlinn, {&Tship});
		DSCar.init(this, &DSLCar, {&TCar});
		//DSSpot.init(this, &DSLSpot, {});
	}

	// Here you destroy your pipelines and Descriptor Sets!
	// All the object classes defined in Starter.hpp have a method .cleanup() for this purpose
	void pipelinesAndDescriptorSetsCleanup() {
		// Cleanup pipelines
		PCar.cleanup();
		PBlinn.cleanup();
		PEmission.cleanup();

		DSGlobal.cleanup();
		DSCar.cleanup();
		DSship.cleanup();
		DSsun.cleanup();
		//DSSpot.cleanup();
	}

	// Here you destroy all the Models, Texture and Desc. Set Layouts you created!
	// All the object classes defined in Starter.hpp have a method .cleanup() for this purpose
	// You also have to destroy the pipelines: since they need to be rebuilt, they have two different
	// methods: .cleanup() recreates them, while .destroy() delete them completely
	void localCleanup() {
		TCar.cleanup();
		MCar.cleanup();

		Tship.cleanup();
		Mship.cleanup();

		Tsun.cleanup();
		Msun.cleanup();
		
		// Cleanup descriptor set layouts
		DSLGlobal.cleanup();
		DSLCar.cleanup();
		DSLBlinn.cleanup();
		DSLEmission.cleanup();
		//DSLSpot.cleanup();
		
		// Destroies the pipelines
		PCar.destroy();	
		PBlinn.destroy();
		PEmission.destroy();
	}
	
	// Here it is the creation of the command buffer:
	// You send to the GPU all the objects you want to draw,
	// with their buffers and textures
	
	void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage) {
		
		PBlinn.bind(commandBuffer);
		Mship.bind(commandBuffer);
		DSGlobal.bind(commandBuffer, PBlinn, 0, currentImage);	// The Global Descriptor Set (Set 0)
		//DSSpot.bind(commandBuffer, PBlinn, 0, currentImage);
		DSship.bind(commandBuffer, PBlinn, 1, currentImage);
		vkCmdDrawIndexed(commandBuffer,
						static_cast<uint32_t>(Mship.indices.size()), NSHIP, 0, 0, 0);	
		


		PCar.bind(commandBuffer);
		MCar.bind(commandBuffer);
		DSGlobal.bind(commandBuffer, PCar, 0, currentImage);
		//DSSpot.bind(commandBuffer, PCar, 0, currentImage);
		DSCar.bind(commandBuffer, PCar, 1, currentImage);
		vkCmdDrawIndexed(commandBuffer,
				static_cast<uint32_t>(MCar.indices.size()), 1, 0, 0, 0);

		PEmission.bind(commandBuffer);
		Msun.bind(commandBuffer);
		DSsun.bind(commandBuffer, PEmission, 0, currentImage);
		vkCmdDrawIndexed(commandBuffer,
			static_cast<uint32_t>(Msun.indices.size()), 1, 0, 0, 0);
	}

	// Here is where you update the uniforms.
	// Very likely this will be where you will be writing the logic of your application.
	void updateUniformBuffer(uint32_t currentImage) {
		//SUN PRAMETERS
		const float turnTime = 72.0f;
		const float angTurnTimeFact = 2.0f * M_PI / turnTime;
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
		
		//GLOBAL PARAMETERS ******************************************************************************************** */
		GlobalUniformBufferObject gubo{};

		float cTime = glfwGetTime();	//TEMPORANEO
		
		gubo.lightDir = glm::vec3(cos(glm::radians(135.0f)) * cos(cTime * angTurnTimeFact), cos(glm::radians(135.0f)) * sin(cTime * angTurnTimeFact),0);
		gubo.lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		gubo.eyePos = glm::vec3(glm::inverse(View) * glm::vec4(0, 0, 0, 1));

		DSGlobal.map(currentImage, &gubo, 0);

		//CAR PARMAT ******************************************************************************************** */
		CarMatParUniformBufferObject uboCarMatPar{};
		uboCarMatPar.Power = 200.0;
		DSCar.map(currentImage, &uboCarMatPar, 2);
		
		CarUniformBufferObject uboCar{};

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

		EmissionUniformBufferObject emissionUbo{};
		emissionUbo.mvpMat = View * glm::translate(glm::mat4(1), gubo.lightDir * 40.0f) * baseTr;
		DSsun.map(currentImage, &emissionUbo, 0);
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