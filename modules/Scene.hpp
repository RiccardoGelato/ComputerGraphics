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
    std::string format;
}ModelScene;

typedef struct  {
    std::string id;
    std::string texture;
}TextureScene;

typedef struct  {
    std::string id;
    std::string model;
    std::string texture;
    glm::mat4 transform;
}InstanceScene;

class SceneProject {
    public:
        //variable used to parse the json file
        std::unordered_map<std::string, ModelScene> modelsParsed;
        std::unordered_map<std::string, TextureScene> texturesParsed;
        std::unordered_map<std::string, InstanceScene> instancesParsed;

        std::unordered_map<std::string, Model> models;
        std::unordered_map<std::string, Texture> textures;

	    BaseProject *BP;
        std::vector <DescriptorSet> DSScene;
    
    void init(BaseProject *_BP, VertexDescriptor *VD, DescriptorSetLayout &DSL, 
			  Pipeline &P, std::string filePath) {
    
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
            ModelScene modelParsed;
            modelParsed.id = modelData["id"];
            modelParsed.model = modelData["model"];
            modelParsed.format = modelData["format"];
            modelsParsed[modelParsed.id] = modelParsed;

            //INITIALIZATION OF MODELS
            Model* model = new Model();
            model->init(BP, VD, modelParsed.model, (modelParsed.format[0] == 'O') ? OBJ : ((modelParsed.format[0] == 'M') ? MGCG : GLTF));
            models[modelParsed.id] = *model;
        }


        // PARSING AND INITIALIZING TEXTURES
        for (const auto& textureData : js["textures"]) {
            TextureScene textureParsed;
            textureParsed.id = textureData["id"];
            textureParsed.texture = textureData["texture"];
            texturesParsed[textureParsed.id] = textureParsed;

            //INITIALIZATION OF TEXTURES
            Texture* texture = new Texture();
            texture->init(BP, textureParsed.texture);
            textures[textureParsed.id] = *texture;
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
        }
    }
    
    void pipelinesAndDescriptorSetsInit(DescriptorSetLayout &DSL) {
		for(auto& instance : instancesParsed) {
            DescriptorSet DS;
            DS.init(BP, &DSL, { &textures[instance.second.texture] });
            DSScene.push_back(DS);
		}
	}

    void pipelinesAndDescriptorSetsCleanup(){
        for(int i = 0; i < instancesParsed.size(); i++) {
            DSScene[i].cleanup();
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

    void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage, Pipeline &P, DescriptorSet &DSGlobal) {
        int i = 0;
        for(auto& instance : instancesParsed) {
            models[instance.second.model].bind(commandBuffer);

            DSGlobal.bind(commandBuffer, P, 0, currentImage);
            DSScene[i].bind(commandBuffer, P, 1, currentImage);

            i++;

            vkCmdDrawIndexed(commandBuffer,
						static_cast<uint32_t>(models[instance.second.model].indices.size()), 1, 0, 0, 0);	
        }
    }


};