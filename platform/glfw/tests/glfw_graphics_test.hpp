#pragma once
#include <functional>
#include <mbgl/map/map.hpp>
#include <queue>

class GLFWGraphicsTest {
public:
    GLFWGraphicsTest() = delete;
    GLFWGraphicsTest(const std::string& testName, const std::string& testDirectory)
        : testName_(testName),
          testDirectory_(testDirectory) {}

    virtual bool initTestFixtures(mbgl::Map* map) = 0;
    virtual bool produceTestCommands(mbgl::Map* map) = 0;
    virtual bool teardownTestFixtures(mbgl::Map* map) = 0;
    virtual int consumeTestCommand(mbgl::Map* map) = 0;

    const std::string& getTestName() const { return testName_; }
    const std::string& getTestDirectory() const { return testDirectory_; }
    uint32_t getCurrentTestCommandCount() const { return testCommands_.size(); }
    virtual ~GLFWGraphicsTest();

protected:
    std::string testName_;
    std::string testDirectory_;
    std::queue<std::function<void(mbgl::Map*)>> testCommands_;
};
