#pragma once

enum class SpriteCollisionType { Clipping,
                                 Wrapping };

struct Config {
    SpriteCollisionType collisionType{SpriteCollisionType::Clipping};
};

class ConfigStorage {
  private:
    inline static Config config{};
    ConfigStorage() = delete;

  public:
    static void SetConfig(Config newConfig) { config = newConfig; };
    static Config getConfig() { return config; };
};
