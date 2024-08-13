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


struct EmissionUniformBufferObject {
	alignas(16) glm::mat4 mvpMat;
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

	//CAMERA PARAMETERS
	const float FOVy = glm::radians(90.0f);
	const float nearPlane = 0.1f;
	const float farPlane = 100.0f;
	float Ar;		// Current aspect ratio (used by the callback that resized the window

	//DISTANCE BETWEEN CAMERA AND CAR
	glm::mat4 Prj;
	glm::vec3 camTarget = glm::vec3(0, 0, 0);
	glm::vec3 camPos = camTarget + glm::vec3(6, 3, 10) / 2.0f;
	glm::mat4 View = glm::lookAt(camPos, camTarget, glm::vec3(0, 1, 0));

	//PARAMETRI PER LA POSIZIONE DELLA MACCHINA E BASE TR
	glm::mat4 baseTr = glm::mat4(1.0f);	
	glm::vec3 Pos = glm::vec3(0.0f, 0.0f, 0.0f);
	float Yaw = 0.0f;

	//********************DESCRIPTOR SET LAYOUT ["classes" of what will be passed to the shaders]
	DescriptorSetLayout DSLCar;
	DescriptorSetLayout DSLGlobal;
	DescriptorSetLayout DSLBlinn;	// For Blinn Objects
	DescriptorSetLayout DSLEmission;

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
		DPSZs.uniformBlocksInPool = 6;
		DPSZs.texturesInPool = 6;
		DPSZs.setsInPool = 4;
		
		Ar = (float)windowWidth / (float)windowHeight;

		//PROJECTION MATRIX
		Prj = glm::perspective(FOVy, Ar, nearPlane, farPlane);
		Prj[1][1] *= -1;
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
		PCar.init(this, &VDCar, "shaders/CarVert.spv", "shaders/CarFrag.spv", {&DSLGlobal, &DSLCar});
		PEmission.init(this, &VDEmission, "shaders/EmissionVert.spv", "shaders/EmissionFrag.spv", { &DSLEmission });

		PBlinn.init(this, &VDBlinn,  "shaders/CarVert.spv",    "shaders/CarFrag.spv", {&DSLGlobal, &DSLBlinn});

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
		DSship.bind(commandBuffer, PBlinn, 1, currentImage);
		vkCmdDrawIndexed(commandBuffer,
						static_cast<uint32_t>(Mship.indices.size()), NSHIP, 0, 0, 0);	
		


		PCar.bind(commandBuffer);
		MCar.bind(commandBuffer);
		DSGlobal.bind(commandBuffer, PCar, 0, currentImage);
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

		//SUN PRAMAETERS
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
		
		//TIMERS AND MOVEMENT CONTROL ******************************************************************************************** */
		float deltaT;
		glm::vec3 m = glm::vec3(0.0f);
		glm::vec3 r = glm::vec3(0.0f);
		bool fire = false;
		getSixAxis(deltaT, m, r, fire);
		
		/* CAMERA MOVEMENT ******************************************************************************************** */
		//Calcolo Yaw e Pitch alla camera
		camYaw += r.y * ROT_SPEED * deltaT;
		camPitch += r.x * ROT_SPEED * deltaT;
		camPitch = glm::clamp(camPitch, glm::radians(-89.0f), glm::radians(89.0f)); //evito che gli assi si sovrappongano

		//Calcolo la posizione della camera (deve sempre putare al target)
		CamPos = CamTarget + glm::vec3(glm::rotate(glm::mat4(1), camYaw, glm::vec3(0,1,0)) * 
                                   glm::rotate(glm::mat4(1), -camPitch, glm::vec3(1,0,0)) * 
                                   glm::vec4(0,0,fixedCamDist,1));
		
		//rendo il movimento della camera più fluido con un esponenziale
		dampedCamPos = CamPos * (1 - exp(-lambdaCam * deltaT)) + dampedCamPos * exp(-lambdaCam * deltaT); 
		
		//creo la matrice di view e projection
		glm::mat4 M = MakeViewProjectionLookAt(dampedCamPos, CamTarget, glm::vec3(0,1,0), 0.0f, glm::radians(90.0f), Ar, 0.1f, 500.0f);
		
		View = M;
		
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
		
		//CONSTANTI PARAMETRI MACCHINA
		const float R = 2.5f;
		const float W = 1.25f;

		//PARAMETRI DELLA MACCHINA NORMALI
		static float carYaw = 0.0f;
		glm::vec3 carDirection = glm::vec3(cos(carYaw), 0, sin(carYaw));

		//angolo di sterzata
		float steeringAngle = m.x * ROT_SPEED * 20 * deltaT * m.z;
		
		//raggio di curvatura
		float turningRadius = R / tan(steeringAngle);

		Pos += m.z * carDirection * MOVE_SPEED * deltaT;
		carYaw -= m.z * (MOVE_SPEED / turningRadius) * deltaT * m.z;
		//carYaw -= m.x * ROT_SPEED * deltaT * m.z; //m.z serve per far girare solo in caso accelleri la macchina

		//rotazione iniziale
		glm::mat4 initialRotation = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0, -1, 0));
		
		//calcolo la dinamica della macchina
		glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), -carYaw, glm::vec3(0, 1, 0));
		glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), Pos);

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
		
	}	


	//PHYSICS FUNCTIONS
	//modify pos and dir (position(x,y,z) and direction(x,y,z)) of the car based on the steering angle and the speed
	void updateCarPositionAndDirection(glm::vec3& pos, glm::vec3& dir, float spe, float steAng, float L, float deltaT) {
		float R; // Turning radius
		float omega;  // Angular velocity

		//new direction vector
		float cosOmegaT;
		float sinOmegaT;
		glm::vec3 newDir;

		R = L / std::tan(steAng);
		omega = spe / R;

		
		cosOmegaT = std::cos(omega * deltaT);
		sinOmegaT = std::sin(omega * deltaT);
		
		
		newDir.x = cosOmegaT * dir.x - sinOmegaT * dir.z;
		newDir.z = sinOmegaT * dir.x + cosOmegaT * dir.z;
		newDir.y = dir.y; // Assuming no change in the y-direction

		// Normalization
		newDir = glm::normalize(newDir);

		// Update the position and direction
		pos += newDir * spe * deltaT;
		dir = newDir;
	}

	//Move the car, todo: implement dynamic
	glm::mat4 MoveCar(glm::vec3 Pos, float Yaw) {
		glm::mat4 M = glm::mat4(1.0f);

		glm::mat4 MT = glm::translate(glm::mat4(1.0f), glm::vec3(Pos[0], 0, Pos[2]));
		glm::mat4 MYaw = glm::rotate(glm::mat4(1.0f), Yaw, glm::vec3(0, 1, 0));
		glm::mat4 MS = glm::scale(glm::mat4(1.0f), glm::vec3(1, 1, 1));
		M = MT * MYaw * MS;

		//std::cout << "Pos: " << Pos[0] << " " << Pos[1] << " " << Pos[2] << std::endl;

		return MT;
	}

	//LookAt Camera function
	glm::mat4 MakeViewProjectionLookAt(glm::vec3 Pos, glm::vec3 Target, glm::vec3 Up, float Roll, float FOVy, float Ar, float nearPlane, float farPlane) {

		glm::mat4 M = glm::mat4(1.0f);
		M = glm::perspective(FOVy, Ar, nearPlane, farPlane);

		glm::mat4 Mv = glm::rotate(glm::mat4(1.0), -Roll, glm::vec3(0, 0, 1)) * glm::scale(glm::mat4(1.0), glm::vec3(1,-1,1))* glm::lookAt(Pos, Target, glm::vec3(Up[0], Up[1], Up[2]));
		M = M * Mv;

		return M;
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