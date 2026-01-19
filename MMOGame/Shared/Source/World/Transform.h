#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace MMO {

struct Transform {
    glm::vec3 position = glm::vec3(0.0f);
    glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    float scale = 1.0f;

    glm::mat4 GetMatrix() const {
        glm::mat4 mat = glm::mat4(1.0f);
        mat = glm::translate(mat, position);
        mat = mat * glm::mat4_cast(rotation);
        mat = glm::scale(mat, glm::vec3(scale));
        return mat;
    }

    glm::vec3 GetForward() const {
        return rotation * glm::vec3(0.0f, 0.0f, -1.0f);
    }

    glm::vec3 GetRight() const {
        return rotation * glm::vec3(1.0f, 0.0f, 0.0f);
    }

    glm::vec3 GetUp() const {
        return rotation * glm::vec3(0.0f, 1.0f, 0.0f);
    }

    void SetEulerAngles(const glm::vec3& eulerDegrees) {
        rotation = glm::quat(glm::radians(eulerDegrees));
    }

    glm::vec3 GetEulerAngles() const {
        return glm::degrees(glm::eulerAngles(rotation));
    }
};

} // namespace MMO
