#include "Param.h"

#include <random>

#include "ParamParser.h"
#include "SimUtilities.h"

namespace sim {

Param* P() {
    return Param::getInstance();
}

// Definition of the variable for linking
Param* Param::INSTANCE = NULL;
Param* Param::getInstance() {
    if (NULL == INSTANCE) {
        INSTANCE = new Param();
    }
    return INSTANCE;
}

Param::Param() {

    // Create the parameter parser object
    ParamParser parser(SimUtilities::getProjectDirectory() + "res/parameters.xml");

    // High priority parameters - these must be initialized before print() will work properly
    m_printWidth = parser.getIntIfHasInt("print-width", 100);
    m_printIdentString = parser.getStringIfHasString("print-indent-string", "    ");

    // Graphical Parameters
    m_initialWindowWidth = parser.getIntIfHasInt("initial-window-width", 930);
    m_initialWindowHeight = parser.getIntIfHasInt("initial-window-height", 470);
    m_windowBorderWidth = parser.getIntIfHasInt("window-border-width", 10);
    m_minZoomedMapScale = parser.getDoubleIfHasDouble("min-zoomed-map-scale", 0.02);
    m_maxZoomedMapScale = parser.getDoubleIfHasDouble("max-zoomed-map-scale", 1.0);
    m_defaultZoomedMapScale = parser.getDoubleIfHasDouble("default-zoomed-map-scale", 0.1);
    m_defaultRotateZoomedMap = parser.getBoolIfHasBool("default-rotate-zoomed-map", false);
    m_frameRate = parser.getIntIfHasInt("frame-rate", 60);
    m_printLateFrames = parser.getBoolIfHasBool("print-late-frames", false);
    m_tileBaseColor = parser.getStringIfHasString("tile-base-color", "BLACK");
    m_tileWallColor = parser.getStringIfHasString("tile-wall-color", "RED");
    m_tileCornerColor = parser.getStringIfHasString("tile-corner-color", "GRAY");
    m_tileTextColor = parser.getStringIfHasString("tile-text-color", "DARK_GREEN");
    m_tileFogColor = parser.getStringIfHasString("tile-fog-color", "GRAY");
    m_tileUndeclaredWallColor = parser.getStringIfHasString("tile-undeclared-wall-color", "DARK_RED");
    m_tileUndeclaredNoWallColor = parser.getStringIfHasString("tile-undeclared-no-wall-color", "DARK_GRAY");
    m_tileIncorrectlyDeclaredWallColor = parser.getStringIfHasString("tile-incorrectly-declared-wall-color", "ORANGE");
    m_tileIncorrectlyDeclaredNoWallColor = parser.getStringIfHasString("tile-incorrectly-declared-no-wall-color", "DARK_CYAN");
    m_mouseBodyColor = parser.getStringIfHasString("mouse-body-color", "BLUE");
    m_mouseWheelColor = parser.getStringIfHasString("mouse-wheel-color", "GREEN");
    m_mouseSensorColor = parser.getStringIfHasString("mouse-sensor-color", "GREEN");
    m_mouseViewColor = parser.getStringIfHasString("mouse-view-color", "WHITE");
    m_defaultMousePathVisible = parser.getBoolIfHasBool("default-mouse-path-visible", true);
    m_defaultWallTruthVisible = parser.getBoolIfHasBool("default-wall-truth-visible", false);
    m_defaultTileColorsVisible = parser.getBoolIfHasBool("default-tile-colors-visible", true);
    m_defaultTileTextVisible = parser.getBoolIfHasBool("default-tile-text-visible", true);
    m_defaultTileFogVisible = parser.getBoolIfHasBool("default-tile-fog-visible", true);
    m_tileFogAlpha = parser.getDoubleIfHasDouble("tile-fog-alpha", 0.15);
    if (m_tileFogAlpha < 0.0 || 1.0 < m_tileFogAlpha) {
        SimUtilities::print("Error: The tile-fog-alpha value of " + std::to_string(m_tileFogAlpha)
            + " is not in the valid range, [0.0, 1.0].");
        SimUtilities::quit();
    }
    m_defaultWireframeMode = parser.getBoolIfHasBool("default-wireframe-mode", false);

    // Simulation Parameters
    bool useRandomSeed = parser.getBoolIfHasBool("use-random-seed", false);
    if (useRandomSeed && !parser.hasIntValue("random-seed")) {
        SimUtilities::print(std::string("Error: The value of use-random-seed is true but no")
            + " valid random-seed value was provided.");
        SimUtilities::quit();
    }
    m_randomSeed = (useRandomSeed ? parser.getIntValue("random-seed") : std::random_device()());
    m_crashMessage = parser.getStringIfHasString("crash-message", "CRASH");
    m_glutInitDuration = parser.getDoubleIfHasDouble("glut-init-duration", 0.1);
    m_defaultPaused = parser.getBoolIfHasBool("default-paused", false);
    m_minSleepDuration = parser.getDoubleIfHasDouble("min-sleep-duration", 5);
    m_discreteInterfaceMinSpeed = parser.getDoubleIfHasDouble("discrete-interface-min-speed", 1.0);
    m_discreteInterfaceMaxSpeed = parser.getDoubleIfHasDouble("discrete-interface-max-speed", 300.0);
    m_discreteInterfaceDefaultSpeed = parser.getDoubleIfHasDouble("discrete-interface-default-speed", 30.0);
    m_discreteInterfaceDeclareWallOnRead = parser.getBoolIfHasBool("discrete-interface-declare-wall-on-read", true);
    m_discreteInterfaceUnfogTileOnEntry = parser.getBoolIfHasBool("discrete-interface-unfog-tile-on-entry", true);
    m_declareBothWallHalves = parser.getBoolIfHasBool("declare-both-wall-halves", true);
    m_mousePositionUpdateRate = parser.getIntIfHasInt("mouse-position-update-rate", 1000);
    m_printLateMousePostitionUpdates = parser.getBoolIfHasBool("print-late-mouse-position-updates", false);
    m_collisionDetectionRate = parser.getIntIfHasInt("collision-detection-rate", 40);
    m_printLateCollisionDetections = parser.getBoolIfHasBool("print-late-collision-detections", false);
    m_printLateSensorReads = parser.getBoolIfHasBool("print-late-sensor-reads", false);
    m_numberOfCircleApproximationPoints = parser.getDoubleIfHasDouble("number-of-circle-approximation-points", 8);
    m_numberOfSensorEdgePoints = parser.getDoubleIfHasDouble("number-of-sensor-edge-points", 3);

    // Maze Parameters
    m_wallWidth = parser.getDoubleIfHasDouble("wall-width", 0.012);
    m_wallLength = parser.getDoubleIfHasDouble("wall-length", 0.168);
    m_wallHeight = parser.getDoubleIfHasDouble("wall-height", 0.05);
    m_enforceOfficialMazeRules = parser.getBoolIfHasBool("enforce-official-maze-rules", false);
    m_mazeDirectory = parser.getStringIfHasString("maze-directory", "res/mazes/");
    m_mazeFile = parser.getStringIfHasString("maze-file", "");
    m_useMazeFile = parser.getBoolIfHasBool("use-maze-file", false);
    m_generatedMazeWidth = parser.getIntIfHasInt("generated-maze-width", 16);
    m_generatedMazeHeight = parser.getIntIfHasInt("generated-maze-height", 16);
    m_mazeAlgorithm = parser.getStringIfHasString("maze-algorithm", "Randomize");
    m_saveGeneratedMaze = parser.getBoolIfHasBool("save-generated-maze", true);

    // Mouse parameters
    m_mouseDirectory = parser.getStringIfHasString("mouse-directory", "res/mice/");
    m_mouseAlgorithm = parser.getStringIfHasString("mouse-algorithm", "RightWallFollow");
}

int Param::initialWindowWidth() {
    return m_initialWindowWidth;
}

int Param::initialWindowHeight() {
    return m_initialWindowHeight;
}

int Param::windowBorderWidth() {
    return m_windowBorderWidth;
}

double Param::minZoomedMapScale() {
    return m_minZoomedMapScale;
}

double Param::maxZoomedMapScale() {
    return m_maxZoomedMapScale;
}

double Param::defaultZoomedMapScale() {
    return m_defaultZoomedMapScale;
}

bool Param::defaultRotateZoomedMap() {
    return m_defaultRotateZoomedMap;
}

int Param::frameRate() {
    return m_frameRate;
}

bool Param::printLateFrames() {
    return m_printLateFrames;
}

std::string Param::tileBaseColor() {
    return m_tileBaseColor;
}

std::string Param::tileWallColor() {
    return m_tileWallColor;
}

std::string Param::tileCornerColor() {
    return m_tileCornerColor;
}

std::string Param::tileTextColor() {
    return m_tileTextColor;
}

std::string Param::tileFogColor() {
    return m_tileFogColor;
}

std::string Param::tileUndeclaredWallColor() {
    return m_tileUndeclaredWallColor;
}

std::string Param::tileUndeclaredNoWallColor() {
    return m_tileUndeclaredNoWallColor;
}

std::string Param::tileIncorrectlyDeclaredWallColor() {
    return m_tileIncorrectlyDeclaredWallColor;
}

std::string Param::tileIncorrectlyDeclaredNoWallColor() {
    return m_tileIncorrectlyDeclaredNoWallColor;
}

std::string Param::mouseBodyColor() {
    return m_mouseBodyColor;
}

std::string Param::mouseWheelColor() {
    return m_mouseWheelColor;
}

std::string Param::mouseSensorColor() {
    return m_mouseSensorColor;
}

std::string Param::mouseViewColor() {
    return m_mouseViewColor;
}

bool Param::defaultMousePathVisible() {
    return m_defaultMousePathVisible;
}

bool Param::defaultWallTruthVisible() {
    return m_defaultWallTruthVisible;
}

bool Param::defaultTileColorsVisible() {
    return m_defaultTileColorsVisible;
}

bool Param::defaultTileTextVisible() {
    return m_defaultTileTextVisible;
}

bool Param::defaultTileFogVisible() {
    return m_defaultTileFogVisible;
}

double Param::tileFogAlpha() {
    return m_tileFogAlpha;
}

bool Param::defaultWireframeMode() {
    return m_defaultWireframeMode;
}

int Param::randomSeed() {
    return m_randomSeed;
}

int Param::printWidth() {
    return m_printWidth;
}

std::string Param::printIndentString() {
    return m_printIdentString;
}

std::string Param::crashMessage() {
    return m_crashMessage;
}

double Param::glutInitDuration() {
    return m_glutInitDuration;
}

bool Param::defaultPaused() {
    return m_defaultPaused;
}

double Param::minSleepDuration() {
    return m_minSleepDuration;
}

double Param::discreteInterfaceMinSpeed() {
    return m_discreteInterfaceMinSpeed;
}

double Param::discreteInterfaceMaxSpeed() {
    return m_discreteInterfaceMaxSpeed;
}

double Param::discreteInterfaceDefaultSpeed() {
    return m_discreteInterfaceDefaultSpeed;
}

bool Param::discreteInterfaceDeclareWallOnRead() {
    return m_discreteInterfaceDeclareWallOnRead;
}

bool Param::discreteInterfaceUnfogTileOnEntry() {
    return m_discreteInterfaceUnfogTileOnEntry;
}

bool Param::declareBothWallHalves() {
    return m_declareBothWallHalves;
}

int Param::mousePositionUpdateRate() {
    return m_mousePositionUpdateRate;
}

bool Param::printLateMousePositionUpdates() {
    return m_printLateMousePostitionUpdates;
}

int Param::collisionDetectionRate() {
    return m_collisionDetectionRate;
}

bool Param::printLateCollisionDetections() {
    return m_printLateCollisionDetections;
}

bool Param::printLateSensorReads() {
    return m_printLateSensorReads;
}

int Param::numberOfCircleApproximationPoints() {
    return m_numberOfCircleApproximationPoints;
}

int Param::numberOfSensorEdgePoints() {
    return m_numberOfSensorEdgePoints;
}

std::string Param::mazeDirectory() {
    return m_mazeDirectory;
}

std::string Param::mazeFile() {
    return m_mazeFile;
}

bool Param::useMazeFile() {
    return m_useMazeFile;
}

double Param::wallWidth() {
    return m_wallWidth;
}

double Param::wallLength() {
    return m_wallLength;
}

double Param::wallHeight() {
    return m_wallHeight;
}

int Param::generatedMazeWidth() {
    return m_generatedMazeWidth;
}

int Param::generatedMazeHeight() {
    return m_generatedMazeHeight;
}

bool Param::enforceOfficialMazeRules() {
    return m_enforceOfficialMazeRules;
}

std::string Param::mazeAlgorithm() {
    return m_mazeAlgorithm;
}

bool Param::saveGeneratedMaze() {
    return m_saveGeneratedMaze;
}

std::string Param::mouseDirectory() {
    return m_mouseDirectory;
}

std::string Param::mouseAlgorithm() {
    return m_mouseAlgorithm;
}

} // namespace sim
