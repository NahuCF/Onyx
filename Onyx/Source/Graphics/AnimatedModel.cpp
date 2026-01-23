#include "pch.h"
#include "AnimatedModel.h"
#include "VertexLayout.h"

#ifdef ONYX_USE_ASSIMP
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#endif

#include <iostream>
#include <filesystem>

namespace Onyx {

#ifdef ONYX_USE_ASSIMP
static glm::mat4 ConvertMatrix(const aiMatrix4x4& m) {
    return glm::mat4(
        m.a1, m.b1, m.c1, m.d1,
        m.a2, m.b2, m.c2, m.d2,
        m.a3, m.b3, m.c3, m.d3,
        m.a4, m.b4, m.c4, m.d4
    );
}
#endif

void SkinnedVertex::AddBoneData(int boneId, float weight) {
    for (int i = 0; i < 4; i++) {
        if (boneIds[i] < 0) {
            boneIds[i] = boneId;
            boneWeights[i] = weight;
            return;
        }
    }
    // If all slots full, replace the smallest weight
    int minIndex = 0;
    float minWeight = boneWeights[0];
    for (int i = 1; i < 4; i++) {
        if (boneWeights[i] < minWeight) {
            minWeight = boneWeights[i];
            minIndex = i;
        }
    }
    if (weight > minWeight) {
        boneIds[minIndex] = boneId;
        boneWeights[minIndex] = weight;
    }
}

bool AnimatedModel::Load(const std::string& path) {
#ifdef ONYX_USE_ASSIMP
    Assimp::Importer importer;

    unsigned int flags = aiProcess_Triangulate
                       | aiProcess_GenSmoothNormals
                       | aiProcess_FlipUVs
                       | aiProcess_CalcTangentSpace
                       | aiProcess_LimitBoneWeights
                       | aiProcess_JoinIdenticalVertices;

    const aiScene* scene = importer.ReadFile(path, flags);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "Assimp error loading " << path << ": " << importer.GetErrorString() << std::endl;
        return false;
    }

    m_Path = path;
    m_Directory = std::filesystem::path(path).parent_path().string();

    // Store global inverse transform
    m_Skeleton.SetGlobalInverseTransform(glm::inverse(ConvertMatrix(scene->mRootNode->mTransformation)));

    // Build node hierarchy first
    BuildNodeHierarchyImpl(scene->mRootNode, -1);

    // Load materials
    LoadMaterialsImpl(scene);

    // Process meshes
    ProcessNodeImpl(scene->mRootNode, scene);

    // Load animations
    LoadAnimationsImpl(scene);

    // Initialize bone matrices
    m_BoneMatrices.resize(m_Skeleton.GetBoneCount(), glm::mat4(1.0f));

    std::cout << "Loaded animated model: " << path << std::endl;
    std::cout << "  Meshes: " << m_Meshes.size() << std::endl;
    std::cout << "  Bones: " << m_Skeleton.GetBoneCount() << std::endl;
    std::cout << "  Animations: " << m_Animations.size() << std::endl;

    return true;
#else
    std::cerr << "AnimatedModel::Load requires ONYX_USE_ASSIMP" << std::endl;
    return false;
#endif
}

#ifdef ONYX_USE_ASSIMP

void AnimatedModel::BuildNodeHierarchyImpl(void* nodePtr, int parentIndex) {
    aiNode* node = static_cast<aiNode*>(nodePtr);
    NodeData nodeData;
    nodeData.name = node->mName.C_Str();
    nodeData.transform = ConvertMatrix(node->mTransformation);
    nodeData.parentIndex = parentIndex;

    int currentIndex = static_cast<int>(m_NodeHierarchy.size());
    m_NodeMap[nodeData.name] = currentIndex;
    m_NodeHierarchy.push_back(nodeData);

    if (parentIndex >= 0) {
        m_NodeHierarchy[parentIndex].children.push_back(currentIndex);
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        BuildNodeHierarchyImpl(node->mChildren[i], currentIndex);
    }
}

void AnimatedModel::ProcessNodeImpl(void* nodePtr, const void* scenePtr) {
    aiNode* node = static_cast<aiNode*>(nodePtr);
    const aiScene* scene = static_cast<const aiScene*>(scenePtr);
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        ProcessMeshImpl(mesh, scene);
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        ProcessNodeImpl(node->mChildren[i], scene);
    }
}

void AnimatedModel::ProcessMeshImpl(void* meshPtr, const void* scenePtr) {
    aiMesh* mesh = static_cast<aiMesh*>(meshPtr);
    const aiScene* scene = static_cast<const aiScene*>(scenePtr);
    std::vector<SkinnedVertex> vertices;
    std::vector<uint32_t> indices;

    // Process vertices
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        SkinnedVertex vertex;

        vertex.position = glm::vec3(
            mesh->mVertices[i].x,
            mesh->mVertices[i].y,
            mesh->mVertices[i].z
        );

        // Update bounds
        m_BoundsMin = glm::min(m_BoundsMin, vertex.position);
        m_BoundsMax = glm::max(m_BoundsMax, vertex.position);

        if (mesh->HasNormals()) {
            vertex.normal = glm::vec3(
                mesh->mNormals[i].x,
                mesh->mNormals[i].y,
                mesh->mNormals[i].z
            );
        }

        if (mesh->mTextureCoords[0]) {
            vertex.texCoords = glm::vec2(
                mesh->mTextureCoords[0][i].x,
                mesh->mTextureCoords[0][i].y
            );
        }

        if (mesh->HasTangentsAndBitangents()) {
            vertex.tangent = glm::vec3(
                mesh->mTangents[i].x,
                mesh->mTangents[i].y,
                mesh->mTangents[i].z
            );
            vertex.bitangent = glm::vec3(
                mesh->mBitangents[i].x,
                mesh->mBitangents[i].y,
                mesh->mBitangents[i].z
            );
        }

        vertices.push_back(vertex);
    }

    // Process indices
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace& face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    // Process bones
    ProcessBonesImpl(mesh, vertices);

    // Normalize bone weights
    for (auto& vertex : vertices) {
        float totalWeight = vertex.boneWeights.x + vertex.boneWeights.y +
                           vertex.boneWeights.z + vertex.boneWeights.w;
        if (totalWeight > 0.0f) {
            vertex.boneWeights /= totalWeight;
        }
        // Replace -1 with 0 for unused bone slots
        for (int i = 0; i < 4; i++) {
            if (vertex.boneIds[i] < 0) {
                vertex.boneIds[i] = 0;
            }
        }
    }

    // Create GPU buffers
    SkinnedMesh skinnedMesh;
    skinnedMesh.name = mesh->mName.C_Str();
    skinnedMesh.materialIndex = mesh->mMaterialIndex;
    skinnedMesh.indexCount = static_cast<uint32_t>(indices.size());

    skinnedMesh.vbo = std::make_unique<VertexBuffer>(
        vertices.data(),
        vertices.size() * sizeof(SkinnedVertex)
    );

    skinnedMesh.ebo = std::make_unique<IndexBuffer>(
        indices.data(),
        indices.size() * sizeof(uint32_t)
    );

    skinnedMesh.vao = std::make_unique<VertexArray>();

    // Layout: pos(3) + normal(3) + uv(2) + tangent(3) + bitangent(3) + boneIds(4i) + weights(4)
    VertexLayout layout;
    layout.PushFloat(3);  // position
    layout.PushFloat(3);  // normal
    layout.PushFloat(2);  // texCoords
    layout.PushFloat(3);  // tangent
    layout.PushFloat(3);  // bitangent
    layout.PushInt(4);    // boneIds
    layout.PushFloat(4);  // boneWeights

    skinnedMesh.vao->SetVertexBuffer(skinnedMesh.vbo.get());
    skinnedMesh.vao->SetIndexBuffer(skinnedMesh.ebo.get());
    skinnedMesh.vao->SetLayout(layout);

    m_Meshes.push_back(std::move(skinnedMesh));
}

void AnimatedModel::ProcessBonesImpl(void* meshPtr, std::vector<SkinnedVertex>& vertices) {
    aiMesh* mesh = static_cast<aiMesh*>(meshPtr);
    for (unsigned int i = 0; i < mesh->mNumBones; i++) {
        aiBone* bone = mesh->mBones[i];
        std::string boneName = bone->mName.C_Str();

        int boneIndex = m_Skeleton.GetBoneIndex(boneName);
        if (boneIndex < 0) {
            // Add new bone
            glm::mat4 offsetMatrix = ConvertMatrix(bone->mOffsetMatrix);

            // Find parent from node hierarchy
            int parentIndex = -1;
            auto nodeIt = m_NodeMap.find(boneName);
            if (nodeIt != m_NodeMap.end()) {
                int nodeIndex = nodeIt->second;
                if (m_NodeHierarchy[nodeIndex].parentIndex >= 0) {
                    std::string parentName = m_NodeHierarchy[m_NodeHierarchy[nodeIndex].parentIndex].name;
                    parentIndex = m_Skeleton.GetBoneIndex(parentName);
                }
            }

            m_Skeleton.AddBone(boneName, parentIndex, offsetMatrix);
            boneIndex = m_Skeleton.GetBoneIndex(boneName);
        }

        // Assign bone weights to vertices
        for (unsigned int j = 0; j < bone->mNumWeights; j++) {
            unsigned int vertexId = bone->mWeights[j].mVertexId;
            float weight = bone->mWeights[j].mWeight;
            if (vertexId < vertices.size()) {
                vertices[vertexId].AddBoneData(boneIndex, weight);
            }
        }
    }
}

void AnimatedModel::LoadAnimationsImpl(const void* scenePtr) {
    const aiScene* scene = static_cast<const aiScene*>(scenePtr);
    for (unsigned int i = 0; i < scene->mNumAnimations; i++) {
        aiAnimation* anim = scene->mAnimations[i];

        std::string animName = anim->mName.C_Str();
        if (animName.empty()) {
            animName = "Animation_" + std::to_string(i);
        }

        float duration = static_cast<float>(anim->mDuration);
        float ticksPerSecond = static_cast<float>(anim->mTicksPerSecond);
        if (ticksPerSecond <= 0.0f) {
            ticksPerSecond = 25.0f;
        }

        auto animation = std::make_unique<Animation>(animName, duration, ticksPerSecond);

        // Process each channel (bone animation)
        for (unsigned int j = 0; j < anim->mNumChannels; j++) {
            aiNodeAnim* channel = anim->mChannels[j];

            BoneAnimation boneAnim;
            boneAnim.boneName = channel->mNodeName.C_Str();

            // Position keys
            for (unsigned int k = 0; k < channel->mNumPositionKeys; k++) {
                PositionKey key;
                key.time = static_cast<float>(channel->mPositionKeys[k].mTime);
                key.position = glm::vec3(
                    channel->mPositionKeys[k].mValue.x,
                    channel->mPositionKeys[k].mValue.y,
                    channel->mPositionKeys[k].mValue.z
                );
                boneAnim.positionKeys.push_back(key);
            }

            // Rotation keys
            for (unsigned int k = 0; k < channel->mNumRotationKeys; k++) {
                RotationKey key;
                key.time = static_cast<float>(channel->mRotationKeys[k].mTime);
                key.rotation = glm::quat(
                    channel->mRotationKeys[k].mValue.w,
                    channel->mRotationKeys[k].mValue.x,
                    channel->mRotationKeys[k].mValue.y,
                    channel->mRotationKeys[k].mValue.z
                );
                boneAnim.rotationKeys.push_back(key);
            }

            // Scale keys
            for (unsigned int k = 0; k < channel->mNumScalingKeys; k++) {
                ScaleKey key;
                key.time = static_cast<float>(channel->mScalingKeys[k].mTime);
                key.scale = glm::vec3(
                    channel->mScalingKeys[k].mValue.x,
                    channel->mScalingKeys[k].mValue.y,
                    channel->mScalingKeys[k].mValue.z
                );
                boneAnim.scaleKeys.push_back(key);
            }

            animation->AddBoneAnimation(boneAnim);
        }

        m_AnimationMap[animName] = static_cast<int>(m_Animations.size());
        m_Animations.push_back(std::move(animation));
    }
}

void AnimatedModel::LoadMaterialsImpl(const void* scenePtr) {
    const aiScene* scene = static_cast<const aiScene*>(scenePtr);
    for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
        aiMaterial* mat = scene->mMaterials[i];

        AnimatedMaterial material;

        aiString name;
        if (mat->Get(AI_MATKEY_NAME, name) == AI_SUCCESS) {
            material.name = name.C_Str();
        }

        aiColor3D color;
        if (mat->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
            material.diffuseColor = glm::vec3(color.r, color.g, color.b);
        }
        if (mat->Get(AI_MATKEY_COLOR_SPECULAR, color) == AI_SUCCESS) {
            material.specularColor = glm::vec3(color.r, color.g, color.b);
        }

        float shininess;
        if (mat->Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS) {
            material.shininess = shininess;
        }

        // Load textures
        aiString texPath;
        if (mat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS) {
            std::string fullPath = m_Directory + "/" + texPath.C_Str();
            material.diffuseTexture = std::make_unique<Texture>(fullPath.c_str());
        }
        if (mat->GetTexture(aiTextureType_NORMALS, 0, &texPath) == AI_SUCCESS) {
            std::string fullPath = m_Directory + "/" + texPath.C_Str();
            material.normalTexture = std::make_unique<Texture>(fullPath.c_str());
        }
        if (mat->GetTexture(aiTextureType_SPECULAR, 0, &texPath) == AI_SUCCESS) {
            std::string fullPath = m_Directory + "/" + texPath.C_Str();
            material.specularTexture = std::make_unique<Texture>(fullPath.c_str());
        }

        m_Materials.push_back(std::move(material));
    }

    // Ensure at least one default material
    if (m_Materials.empty()) {
        AnimatedMaterial defaultMat;
        defaultMat.name = "Default";
        m_Materials.push_back(std::move(defaultMat));
    }
}

#endif // ONYX_USE_ASSIMP

Animation* AnimatedModel::GetAnimation(const std::string& name) {
    auto it = m_AnimationMap.find(name);
    if (it != m_AnimationMap.end()) {
        return m_Animations[it->second].get();
    }
    return nullptr;
}

Animation* AnimatedModel::GetAnimation(int index) {
    if (index >= 0 && index < static_cast<int>(m_Animations.size())) {
        return m_Animations[index].get();
    }
    return nullptr;
}

std::vector<std::string> AnimatedModel::GetAnimationNames() const {
    std::vector<std::string> names;
    names.reserve(m_Animations.size());
    for (const auto& anim : m_Animations) {
        names.push_back(anim->GetName());
    }
    return names;
}

} // namespace Onyx
