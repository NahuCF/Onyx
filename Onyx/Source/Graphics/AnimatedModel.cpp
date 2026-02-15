#include "pch.h"
#include "AnimatedModel.h"
#include "VertexLayout.h"

#ifdef ONYX_USE_ASSIMP
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/config.h>
#endif

#include <iostream>
#include <filesystem>

namespace Onyx {

AnimatedModel::~AnimatedModel() = default;

#ifdef ONYX_USE_ASSIMP
static glm::mat4 ConvertMatrix(const aiMatrix4x4& from) {
    glm::mat4 to;
    to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
    to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
    to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
    to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
    return to;
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

    importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);

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

    m_Skeleton.SetGlobalInverseTransform(glm::inverse(ConvertMatrix(scene->mRootNode->mTransformation)));
    BuildNodeHierarchyImpl(scene->mRootNode, -1);
    LoadMaterialsImpl(scene);
    ProcessNodeImpl(scene->mRootNode, scene);
    LoadAnimationsImpl(scene);

    for (int i = 0; i < m_Skeleton.GetBoneCount(); i++) {
        const Bone* bone = m_Skeleton.GetBone(i);
        if (bone && bone->parentIndex < 0) {
            auto nodeIt = m_NodeMap.find(bone->name);
            if (nodeIt != m_NodeMap.end()) {
                int nodeIndex = nodeIt->second;
                int parentNodeIndex = m_NodeHierarchy[nodeIndex].parentIndex;
                while (parentNodeIndex >= 0) {
                    const std::string& parentName = m_NodeHierarchy[parentNodeIndex].name;
                    int parentBoneIndex = m_Skeleton.GetBoneIndex(parentName);
                    if (parentBoneIndex >= 0) {
                        m_Skeleton.SetBoneParent(i, parentBoneIndex);
                        break;
                    }
                    parentNodeIndex = m_NodeHierarchy[parentNodeIndex].parentIndex;
                }
            }
        }
    }

    m_BoneMatrices.resize(m_Skeleton.GetBoneCount(), glm::mat4(1.0f));

    return true;
#else
    std::cerr << "AnimatedModel::Load requires ONYX_USE_ASSIMP" << std::endl;
    return false;
#endif
}

bool AnimatedModel::LoadAnimation(const std::string& path) {
#ifdef ONYX_USE_ASSIMP
    if (m_Skeleton.GetBoneCount() == 0) {
        std::cerr << "AnimatedModel::LoadAnimation: Must load a model with skeleton first" << std::endl;
        return false;
    }

    Assimp::Importer importer;
    importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);

    unsigned int flags = aiProcess_Triangulate;

    const aiScene* scene = importer.ReadFile(path, flags);

    if (!scene || !scene->mRootNode) {
        std::cerr << "Assimp error loading animation " << path << ": " << importer.GetErrorString() << std::endl;
        return false;
    }

    if (scene->mNumAnimations == 0) {
        std::cerr << "No animations found in " << path << std::endl;
        return false;
    }

    std::string baseAnimName = std::filesystem::path(path).stem().string();

    int animationsLoaded = 0;
    for (unsigned int i = 0; i < scene->mNumAnimations; i++) {
        aiAnimation* anim = scene->mAnimations[i];

        std::string animName = baseAnimName;
        if (scene->mNumAnimations > 1) {
            animName += "_" + std::to_string(i);
        }

        if (m_AnimationMap.find(animName) != m_AnimationMap.end()) {
            std::cerr << "Animation '" << animName << "' already exists, skipping" << std::endl;
            continue;
        }

        float duration = static_cast<float>(anim->mDuration);
        float ticksPerSecond = static_cast<float>(anim->mTicksPerSecond);
        if (ticksPerSecond <= 0.0f) {
            ticksPerSecond = 25.0f;
        }

        auto animation = std::make_unique<Animation>(animName, duration, ticksPerSecond);

        int matchingBones = 0;
        for (unsigned int j = 0; j < anim->mNumChannels; j++) {
            aiNodeAnim* channel = anim->mChannels[j];
            std::string boneName = channel->mNodeName.C_Str();

            if (m_Skeleton.GetBoneIndex(boneName) < 0) {
                continue;
            }
            matchingBones++;

            BoneAnimation boneAnim;
            boneAnim.boneName = boneName;

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

        if (matchingBones == 0) {
            std::cerr << "Warning: Animation '" << animName << "' has no bones matching skeleton" << std::endl;
            continue;
        }

        m_AnimationMap[animName] = static_cast<int>(m_Animations.size());
        m_Animations.push_back(std::move(animation));
        animationsLoaded++;

        std::cout << "Loaded animation '" << animName << "' with " << matchingBones << " bones" << std::endl;
    }

    return animationsLoaded > 0;
#else
    std::cerr << "AnimatedModel::LoadAnimation requires ONYX_USE_ASSIMP" << std::endl;
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

    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        SkinnedVertex vertex;

        vertex.position = glm::vec3(
            mesh->mVertices[i].x,
            mesh->mVertices[i].y,
            mesh->mVertices[i].z
        );

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

    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace& face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    ProcessBonesImpl(mesh, vertices);

    for (auto& vertex : vertices) {
        float totalWeight = vertex.boneWeights.x + vertex.boneWeights.y +
                           vertex.boneWeights.z + vertex.boneWeights.w;
        if (totalWeight > 0.0f) {
            vertex.boneWeights /= totalWeight;
        }
        for (int i = 0; i < 4; i++) {
            if (vertex.boneIds[i] < 0) {
                vertex.boneIds[i] = 0;
            }
        }
    }

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

    if (skinnedMesh.vao->GetBufferID() == 0) {
        std::cerr << "Failed to create VAO for mesh: " << mesh->mName.C_Str() << std::endl;
        return;
    }

    VertexLayout layout;
    layout.PushFloat(3);
    layout.PushFloat(3);
    layout.PushFloat(2);
    layout.PushFloat(3);
    layout.PushFloat(3);
    layout.PushInt(4);
    layout.PushFloat(4);

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
            glm::mat4 offsetMatrix = ConvertMatrix(bone->mOffsetMatrix);
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

        for (unsigned int j = 0; j < anim->mNumChannels; j++) {
            aiNodeAnim* channel = anim->mChannels[j];

            BoneAnimation boneAnim;
            boneAnim.boneName = channel->mNodeName.C_Str();

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
