//#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

//define Structures

typedef struct  {
    std::string id;
    std::string model;
    std::string texture;
    glm::mat4 transform;
}InstanceScene;

class SceneProject {
    public:
        //variable used to parse the json file
        std::unordered_map<std::string, InstanceScene> instancesParsed;
        std::unordered_map<std::string, InstanceScene> instancesParsedCopy;

        std::unordered_map<std::string, Model> models;
        std::unordered_map<std::string, Texture> textures;


	    BaseProject *BP;
        DescriptorSet **DSScene;
    
    void init(BaseProject *_BP, VertexDescriptor *VDBlinn, DescriptorSetLayout &DSLBlinn, 
			  Pipeline &PBlinn, std::string filePath) {
        BP = _BP;

        //provo ad aprire il file
        nlohmann::json js;
        std::ifstream file(filePath);

        //se non riesco restituisco errore
        if (!file.is_open()) {
            std::cerr << "Could not open the file!" << std::endl;
            return;
        }

        file >> js;
        file.close();

        // PARSING AND INITIALIZING MODELS
        for (const auto& modelData : js["models"]) {
			std::string format = modelData["format"];
            //INITIALIZATION OF MODELS
            Model model;
            model.init(BP, VDBlinn, modelData["model"], (format[0] == 'O') ? OBJ : ((format[0] == 'M') ? MGCG : GLTF));
            models[modelData["id"]] = model;
        }


        // PARSING AND INITIALIZING TEXTURES
        for (const auto& textureData : js["textures"]) {

            //INITIALIZATION OF TEXTURES
            Texture texture;
            texture.init(BP, textureData["texture"]);
            textures[textureData["id"]] = texture;
        }

        //INITIALIZING INSTANCES
        for (const auto& instanceData : js["instances"]) {
            InstanceScene instance;
            instance.texture = instanceData["texture"];
            instance.model = instanceData["model"];
            
            std::vector<float> transformArray = instanceData.value("transform", std::vector<float>(16, 0.0f));
            if (transformArray.size() == 16) {
                instance.transform = glm::make_mat4(transformArray.data());
            }

            instancesParsed[instanceData["id"]] = instance;
            instancesParsedCopy[instanceData["id"]] = instance;

            DSScene = (DescriptorSet **)calloc(instancesParsed.size(), sizeof(DescriptorSet *));
        }
    }
   
    
    void pipelinesAndDescriptorSetsInit(DescriptorSetLayout &DSL) {
		int i = 0;
        for(auto& instance : instancesParsed) {
            
            DSScene[i] = new DescriptorSet();
            DSScene[i]->init(BP, &DSL, { &textures[instance.second.texture], &textures[instance.second.texture], &textures[instance.second.texture] });
            i++;
		}
	}

    void pipelinesAndDescriptorSetsCleanup(){
        for(int i = 0; i < instancesParsed.size(); i++) {
            DSScene[i]->cleanup();
            delete DSScene[i];
        }
    }

    void localCleanup(){
        for (auto& model : models) {
            model.second.cleanup();
        }
        for (auto& texture : textures) {
            texture.second.cleanup();
        }
    }

    void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage, Pipeline PBlinn, DescriptorSet &DSGlobal) {
        int i = 0;
       
        for(auto& instance : instancesParsed) {
            models[instance.second.model].bind(commandBuffer);

            DSGlobal.bind(commandBuffer, PBlinn, 0, currentImage);
			DSScene[i]->bind(commandBuffer, PBlinn, 1, currentImage);

            vkCmdDrawIndexed(commandBuffer,
						static_cast<uint32_t>(models[instance.second.model].indices.size()), 1, 0, 0, 0);	
			i++;	
        }
    }
    

};