#pragma once

#include <cstdint>
#include <functional>

namespace Onyx {

class Model;
class AnimatedModel;
class Texture;

template<typename T>
struct AssetHandle {
    uint32_t id = 0;
    bool IsValid() const { return id != 0; }
    bool operator==(const AssetHandle& o) const { return id == o.id; }
    bool operator!=(const AssetHandle& o) const { return id != o.id; }
};

using ModelHandle         = AssetHandle<Model>;
using AnimatedModelHandle = AssetHandle<AnimatedModel>;
using TextureHandle       = AssetHandle<Texture>;

} // namespace Onyx

namespace std {
template<typename T>
struct hash<Onyx::AssetHandle<T>> {
    size_t operator()(const Onyx::AssetHandle<T>& h) const {
        return std::hash<uint32_t>()(h.id);
    }
};
}
