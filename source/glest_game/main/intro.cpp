// ==============================================================
//      This file is part of Glest (www.glest.org)
//
//      Copyright (C) 2001-2008 Martiño Figueroa
//
//      You can redistribute this code and/or modify it under
//      the terms of the GNU General Public License as published
//      by the Free Software Foundation; either version 2 of the
//      License, or (at your option) any later version
// ==============================================================
#include "intro.h"

#include "main_menu.h"
#include "util.h"
#include "game_util.h"
#include "config.h"
#include "program.h"
#include "renderer.h"
#include "sound_renderer.h"
#include "core_data.h"
#include "metrics.h"
#include "auto_test.h"
#include "util.h"
//#include "glm.h"
//#include "md5util.h"
//#include "Mathlib.h"

#include "video_player.h"

#include "leak_dumper.h"

using namespace
Shared::Util;
using
namespace
	Shared::Graphics;
using
namespace
	Shared::Xml;

namespace Glest {
	namespace Game {

		//struct Timer {
		//public:
		//  Timer ()
		//    : current_time (0.0), last_time (0.0) { }
		//
		//public:
		//  void update () {
		//    last_time = current_time;
		//    current_time = static_cast<double>(SDL_GetTicks ()) / 1000.0;
		//  }
		//
		//  double deltaTime () const {
		//    return (current_time - last_time);
		//  }
		//
		//public:
		//  double current_time;
		//  double last_time;
		//
		//} animTimer;

		// =====================================================
		//      class Text
		// =====================================================

		IntroText::IntroText(const string & text, const Vec2i & pos, int time,
			Font2D * font, Font3D * font3D) {
			this->text = text;
			this->pos = pos;
			this->time = time;
			this->texture = NULL;
			this->font = font;
			this->font3D = font3D;
		}

		IntroText::IntroText(const Texture2D * texture, const Vec2i & pos,
			const Vec2i & size, int time) {
			this->pos = pos;
			this->size = size;
			this->time = time;
			this->texture = texture;
			this->font = NULL;
			this->font3D = NULL;
		}

		// =====================================================
		//      class Intro
		// =====================================================

		int
			Intro::introTime = 50000;
		int
			Intro::appearTime = 2500;
		int
			Intro::showTime = 3500;
		int
			Intro::disapearTime = 2500;

		Intro::Intro(Program * program) :
			ProgramState(program) {
			if (SystemFlags::
				getSystemSettingType(SystemFlags::debugSystem).enabled)
				SystemFlags::OutputDebug(SystemFlags::debugSystem,
					"In [%s::%s Line: %d]\n", __FILE__,
					__FUNCTION__, __LINE__);

			CoreData & coreData = CoreData::getInstance();
			SoundRenderer & soundRenderer = SoundRenderer::getInstance();

			if (GlobalStaticFlags::getIsNonGraphicalModeEnabled() == false &&
				(::Shared::Graphics::VideoPlayer::hasBackEndVideoPlayer() == false
					|| CoreData::getInstance().hasIntroVideoFilename() == false)) {
				soundRenderer.playMusic(CoreData::getInstance().getIntroMusic());
			}

			const
				Metrics &
				metrics = Metrics::getInstance();
			int
				w = metrics.getVirtualW();
			int
				h = metrics.getVirtualH();
			timer = 0;
			mouseX = 0;
			mouseY = 0;
			mouse2d = 0;
			exitAfterIntroVideo = false;

			Renderer & renderer = Renderer::getInstance();
			//renderer.init3dListMenu(NULL);
			renderer.initMenu(NULL);
			fade = 0.f;
			anim = 0.f;
			targetCamera = NULL;
			t = 0.f;

			startPosition.x = 5;;
			startPosition.y = 10;
			startPosition.z = 40;
			camera.setPosition(startPosition);

			Vec3f startRotation;
			startRotation.x = 0;
			startRotation.y = 0;
			startRotation.z = 0;

			camera.setOrientation(Quaternion
			(EulerAngles
			(degToRad(startRotation.x),
				degToRad(startRotation.y),
				degToRad(startRotation.z))));

			Intro::introTime = 3000;
			Intro::appearTime = 500;
			Intro::showTime = 500;
			Intro::disapearTime = 500;
			int
				showIntroPics = 0;
			int
				showIntroPicsTime = 0;
			bool showIntroPicsRandom = false;
			bool showIntroModels = false;
			bool showIntroModelsRandom = false;
			modelMinAnimSpeed = 0;
			modelMaxAnimSpeed = 0;

			XmlTree xmlTree;
			string
				data_path =
				getGameReadWritePath(GameConstants::path_data_CacheLookupKey);
			xmlTree.load(getGameCustomCoreDataPath
			(data_path, "data/core/menu/menu.xml"),
				Properties::getTagReplacementValues());
			const XmlNode *
				menuNode = xmlTree.getRootNode();

			if (menuNode->hasChild("intro") == true) {
				const XmlNode *
					introNode = menuNode->getChild("intro");

				if (SystemFlags::
					getSystemSettingType(SystemFlags::debugSystem).enabled)
					SystemFlags::OutputDebug(SystemFlags::debugSystem,
						"In [%s::%s %d]\n", __FILE__,
						__FUNCTION__, __LINE__);

				//camera
				const XmlNode *
					cameraNode = introNode->getChild("camera");

				//position
				const XmlNode *
					positionNode = cameraNode->getChild("start-position");
				startPosition.x = positionNode->getAttribute("x")->getFloatValue();
				startPosition.y = positionNode->getAttribute("y")->getFloatValue();
				startPosition.z = positionNode->getAttribute("z")->getFloatValue();
				camera.setPosition(startPosition);

				//rotation
				const XmlNode *
					rotationNode = cameraNode->getChild("start-rotation");
				Vec3f startRotation;
				startRotation.x = rotationNode->getAttribute("x")->getFloatValue();
				startRotation.y = rotationNode->getAttribute("y")->getFloatValue();
				startRotation.z = rotationNode->getAttribute("z")->getFloatValue();
				camera.setOrientation(Quaternion
				(EulerAngles
				(degToRad(startRotation.x),
					degToRad(startRotation.y),
					degToRad(startRotation.z))));

				// intro info
				const XmlNode *
					introTimeNode = introNode->getChild("intro-time");
				Intro::introTime =
					introTimeNode->getAttribute("value")->getIntValue();
				const XmlNode *
					appearTimeNode = introNode->getChild("appear-time");
				Intro::appearTime =
					appearTimeNode->getAttribute("value")->getIntValue();
				const XmlNode *
					showTimeNode = introNode->getChild("show-time");
				Intro::showTime =
					showTimeNode->getAttribute("value")->getIntValue();
				const XmlNode *
					disappearTimeNode = introNode->getChild("disappear-time");
				Intro::disapearTime =
					disappearTimeNode->getAttribute("value")->getIntValue();
				const XmlNode *
					showIntroPicturesNode = introNode->getChild("show-intro-pictures");
				showIntroPics =
					showIntroPicturesNode->getAttribute("value")->getIntValue();
				showIntroPicsTime =
					showIntroPicturesNode->getAttribute("time")->getIntValue();
				showIntroPicsRandom =
					showIntroPicturesNode->getAttribute("random")->getBoolValue();

				const XmlNode *
					showIntroModelsNode = introNode->getChild("show-intro-models");
				showIntroModels =
					showIntroModelsNode->getAttribute("value")->getBoolValue();
				showIntroModelsRandom =
					showIntroModelsNode->getAttribute("random")->getBoolValue();
				modelMinAnimSpeed =
					showIntroModelsNode->
					getAttribute("min-anim-speed")->getFloatValue();
				modelMaxAnimSpeed =
					showIntroModelsNode->
					getAttribute("max-anim-speed")->getFloatValue();
			}

			//load main model
			modelIndex = 0;
			models.clear();
			if (showIntroModels == true) {

				//getGameCustomCoreDataPath(data_path, "data/core/menu/menu.xml")
				string
					introPath =
					getGameCustomCoreDataPath(data_path,
						"") +
					"data/core/menu/main_model/intro*.g3d";
				vector < string > introModels;
				findAll(introPath, introModels, false, false);
				for (int i = 0; i < (int) introModels.size(); ++i) {
					string logo = introModels[i];
					Model *
						model = renderer.newModel(rsMenu,
							getGameCustomCoreDataPath(data_path,
								"") +
							"data/core/menu/main_model/" + logo);
					if (model) {
						models.push_back(model);
						//printf("#1 Intro model [%s]\n",model->getFileName().c_str());
					}
				}

				if (models.empty() == true) {
					introPath = data_path + "data/core/menu/main_model/intro*.g3d";
					//vector<string> introModels;
					findAll(introPath, introModels, false, false);
					for (int i = 0; i < (int) introModels.size(); ++i) {
						string logo = introModels[i];
						Model *
							model = renderer.newModel(rsMenu,
								data_path +
								"data/core/menu/main_model/" + logo);
						if (model) {
							models.push_back(model);
							//printf("#2 Intro model [%s]\n",model->getFileName().c_str());
						}
					}
				}

				if (showIntroModelsRandom == true) {
					std::vector < Model * >modelList;

					//unsigned int seed = time(NULL);
					Chrono seed(true);
					srand((unsigned int) seed.getCurTicks());
					int
						failedLookups = 0;
					std::map < int,
						bool >
						usedIndex;
					for (; modelList.size() < models.size();) {
						int
							index = rand() % models.size();
						if (usedIndex.find(index) != usedIndex.end()) {
							failedLookups++;
							srand((unsigned int) seed.getCurTicks() / failedLookups);
							continue;
						}
						//printf("picIndex = %d list count = %d\n",picIndex,coreData.getMiscTextureList().size());
						modelList.push_back(models[index]);
						usedIndex[index] = true;
						srand((unsigned int) seed.getCurTicks() /
							(unsigned int) modelList.size());
					}
					models = modelList;
				}
			}

			int
				displayItemNumber = 1;
			int
				appear = Intro::appearTime;
			int
				disappear =
				Intro::showTime + Intro::appearTime + (Intro::disapearTime * 2);

			const unsigned int
				maxIntroLines = 100;
			Lang & lang = Lang::getInstance();
			for (unsigned int i = 1; i < maxIntroLines; ++i) {
				string introTagName = "IntroText" + intToStr(i);
				string introTagTextureName = "IntroTexture" + intToStr(i);

				if (lang.hasString(introTagName, "", true) == true ||
					lang.hasString(introTagTextureName, "", true) == true) {
					string lineText = "";

					if (lang.hasString(introTagName, "", true) == true) {
						lineText = lang.getString(introTagName, "");
					}

					string showStartTime = "IntroStartMilliseconds" + intToStr(i);

					int
						displayTime = appear;
					if (lang.hasString(showStartTime, "", true) == true) {
						displayTime = strToInt(lang.getString(showStartTime, ""));
					} else {
						if (i == 1) {
							displayTime = appear;
						} else if (i == 2) {
							displayTime = disappear;
						} else if (i >= 3) {
							displayTime = disappear * (++displayItemNumber);
						}
					}

					// Is this a texture?
					if (lang.hasString(introTagName, "", true) == false &&
						lang.hasString(introTagTextureName, "", true) == true) {

						string
							introTagTextureWidthName = "IntroTextureWidth" + intToStr(i);
						string
							introTagTextureHeightName = "IntroTextureHeight" + intToStr(i);

						lineText = lang.getString(introTagTextureName, "");
						Texture2D *
							logoTexture = renderer.newTexture2D(rsGlobal);
						if (logoTexture) {
							logoTexture->setMipmap(false);
							logoTexture->getPixmap()->load(lineText);

							renderer.initTexture(rsGlobal, logoTexture);
						}


						int
							textureWidth = 256;
						if (logoTexture != NULL) {
							textureWidth = logoTexture->getTextureWidth();
						}
						if (lang.hasString(introTagTextureWidthName, "", true) == true) {
							textureWidth =
								strToInt(lang.getString
								(introTagTextureWidthName, ""));
						}

						int
							textureHeight = 128;
						if (logoTexture != NULL) {
							textureHeight = logoTexture->getTextureHeight();
						}
						if (lang.hasString(introTagTextureHeightName, "", true) == true) {
							textureHeight =
								strToInt(lang.getString
								(introTagTextureHeightName, ""));
						}

						texts.push_back(new
							IntroText(logoTexture,
								Vec2i(w / 2 - (textureWidth / 2),
									h / 2 - (textureHeight / 2)),
								Vec2i(textureWidth, textureHeight),
								displayTime));
					}
					// This is a line of text
					else {
						string introTagTextXName = "IntroTextX" + intToStr(i);
						string introTagTextYName = "IntroTextY" + intToStr(i);
						string
							introTagTextFontTypeName = "IntroTextFontType" + intToStr(i);

						int
							textX = -1;
						if (lang.hasString(introTagTextXName, "", true) == true) {
							string value = lang.getString(introTagTextXName, "");
							if (value.length() > 0 && (value[0] == '+' || value[0] == '-')) {
								textX = w / 2 + strToInt(value);
							} else {
								textX = strToInt(value);
							}
						}

						int
							textY = -1;
						if (lang.hasString(introTagTextYName, "", true) == true) {
							string value = lang.getString(introTagTextYName, "");
							if (value.length() > 0 && (value[0] == '+' || value[0] == '-')) {
								textY = h / 2 + strToInt(value);
							} else {
								textY = strToInt(value);
							}
						}

						Font2D *
							font = coreData.getMenuFontVeryBig();
						Font3D *
							font3d = coreData.getMenuFontVeryBig3D();

						if (lang.hasString(introTagTextFontTypeName, "", true) == true) {
							string
								value = lang.getString(introTagTextFontTypeName, "");
							if (value == "displaynormal") {
								font = coreData.getDisplayFont();
								font3d = coreData.getDisplayFont3D();
							} else if (value == "displaysmall") {
								font = coreData.getDisplayFontSmall();
								font3d = coreData.getDisplayFontSmall3D();
							} else if (value == "menunormal") {
								font = coreData.getMenuFontNormal();
								font3d = coreData.getMenuFontNormal3D();
							} else if (value == "menubig") {
								font = coreData.getMenuFontBig();
								font3d = coreData.getMenuFontBig3D();
							} else if (value == "menuverybig") {
								font = coreData.getMenuFontVeryBig();
								font3d = coreData.getMenuFontVeryBig3D();
							} else if (value == "consolenormal") {
								font = coreData.getConsoleFont();
								font3d = coreData.getConsoleFont3D();
							}

						}
						texts.push_back(new
							IntroText(lineText, Vec2i(textX, textY),
								displayTime, font, font3d));
					}
				} else {
					break;
				}
			}
			modelShowTime = disappear * (displayItemNumber);
			if (lang.hasString("IntroModelStartMilliseconds", "", true) == true) {
				modelShowTime =
					strToInt(lang.getString("IntroModelStartMilliseconds", ""));
			} else {
				modelShowTime = disappear * (displayItemNumber);
			}

			/*
			string lineText = "Based on award-winning classic Glest";
			texts.push_back(new Text(lineText, Vec2i(-1, -1), appear, coreData.getMenuFontVeryBig(),coreData.getMenuFontVeryBig3D()));
			lineText = "the MegaGlest Team presents";
			texts.push_back(new Text(lineText, Vec2i(-1, -1), disappear, coreData.getMenuFontVeryBig(),coreData.getMenuFontVeryBig3D()));
			lineText = "a libre software real-time strategy game";
			texts.push_back(new Text(lineText, Vec2i(-1, -1), disappear *(++displayItemNumber), coreData.getMenuFontVeryBig(),coreData.getMenuFontVeryBig3D()));

			texts.push_back(new Text(coreData.getLogoTexture(), Vec2i(w/2-128, h/2-64), Vec2i(256, 128), disappear *(++displayItemNumber)));
			texts.push_back(new Text(GAME_VERSION, Vec2i(w/2+45, h/2-45), disappear *(displayItemNumber++), coreData.getMenuFontNormal(),coreData.getMenuFontNormal3D()));
			lineText = "www.megaglest.org";
			//texts.push_back(new Text(lineText, Vec2i(-1, -1), disappear *(displayItemNumber++), coreData.getMenuFontVeryBig(),coreData.getMenuFontVeryBig3D()));
			texts.push_back(new Text(lineText, Vec2i(-1, h/2-45-18), disappear *(displayItemNumber-1), coreData.getMenuFontVeryBig(),coreData.getMenuFontVeryBig3D()));
			*/



			if (showIntroPics > 0 && coreData.getMiscTextureList().size() > 0) {
				const int
					showMiscTime = showIntroPicsTime;

				std::vector < Texture2D * >intoTexList;
				if (showIntroPicsRandom == true) {
					//unsigned int seed = time(NULL);
					Chrono seed(true);
					srand((unsigned int) seed.getCurTicks());
					int
						failedLookups = 0;
					std::map < int,
						bool >
						usedIndex;
					for (; (int) intoTexList.size() < showIntroPics;) {
						int
							picIndex = rand() % coreData.getMiscTextureList().size();
						if (usedIndex.find(picIndex) != usedIndex.end()) {
							failedLookups++;
							srand((unsigned int) seed.getCurTicks() / failedLookups);

							if (failedLookups > 10000) {
								for (unsigned int i = 0;
									i < coreData.getMiscTextureList().size(); ++i) {
									if (usedIndex.find(i) == usedIndex.end()) {
										picIndex = i;
										break;
									}
								}
							} else {
								continue;
							}
						}
						//printf("picIndex = %d list count = %d\n",picIndex,coreData.getMiscTextureList().size());
						intoTexList.push_back(coreData.getMiscTextureList()[picIndex]);
						usedIndex[picIndex] = true;
						srand((unsigned int) seed.getCurTicks() /
							(unsigned int) intoTexList.size());
					}
				} else {
					for (unsigned int i = 0;
						i < (unsigned int) coreData.getMiscTextureList().size() &&
						i < (unsigned int) showIntroPics; ++i) {
						Texture2D *
							tex = coreData.getMiscTextureList()[i];
						intoTexList.push_back(tex);
					}
				}

				for (unsigned int i = 0; i < intoTexList.size(); ++i) {
					Texture2D *
						tex = intoTexList[i];
					//printf("tex # %d [%s]\n",i,tex->getPath().c_str());
					if (tex != NULL) {
						Vec2i texPlacement;
						if (i == 0 || i % 9 == 0) {
							texPlacement = Vec2i(1, h - tex->getTextureHeight());
						} else if (i == 1 || i % 9 == 1) {
							texPlacement = Vec2i(1, 1);
						} else if (i == 2 || i % 9 == 2) {
							texPlacement = Vec2i(w - tex->getTextureWidth(), 1);
						} else if (i == 3 || i % 9 == 3) {
							texPlacement =
								Vec2i(w - tex->getTextureWidth(),
									h - tex->getTextureHeight());
						} else if (i == 4 || i % 9 == 4) {
							texPlacement =
								Vec2i(w / 2 - tex->getTextureWidth() / 2,
									h - tex->getTextureHeight());
						} else if (i == 5 || i % 9 == 5) {
							texPlacement = Vec2i(w / 2 - tex->getTextureWidth() / 2, 1);
						} else if (i == 6 || i % 9 == 6) {
							texPlacement =
								Vec2i(1, (h / 2) - (tex->getTextureHeight() / 2));
						} else if (i == 7 || i % 9 == 7) {
							texPlacement =
								Vec2i(w - tex->getTextureWidth(),
								(h / 2) - (tex->getTextureHeight() / 2));
						}

						int
							textureStartTime = disappear * displayItemNumber;
						if (lang.hasString("IntroTextureStartMilliseconds", "", true) ==
							true) {
							textureStartTime =
								strToInt(lang.getString("IntroTextureStartMilliseconds", ""));
						}

						texts.push_back(new
							IntroText(tex, texPlacement,
								Vec2i(tex->getTextureWidth(),
									tex->getTextureHeight()),
								textureStartTime +
								(showMiscTime * (i + 1))));
					}
				}
			}

			if (GlobalStaticFlags::getIsNonGraphicalModeEnabled() == false) {
				renderer.swapBuffers();
			}

			SystemFlags::OutputDebug(SystemFlags::debugSystem,
				"In [%s::%s Line: %d]\n", __FILE__,
				__FUNCTION__, __LINE__);

			if (GlobalStaticFlags::getIsNonGraphicalModeEnabled() == false
				&& ::Shared::Graphics::VideoPlayer::hasBackEndVideoPlayer() == true
				&& CoreData::getInstance().hasIntroVideoFilename() == true) {
				string
					introVideoFile = CoreData::getInstance().getIntroVideoFilename();
				string
					introVideoFileFallback =
					CoreData::getInstance().getIntroVideoFilenameFallback();

				if (SystemFlags::VERBOSE_MODE_ENABLED)
					printf("Intro Video [%s] [%s]\n", introVideoFile.c_str(),
						introVideoFileFallback.c_str());

				Context *
					c = GraphicsInterface::getInstance().getCurrentContext();
				PlatformContextGl *
					glCtx = static_cast <ContextGl *>(c)->getPlatformContextGlPtr();
				SDL_Window *
					window = glCtx->getScreenWindow();
				SDL_Surface *
					screen = glCtx->getScreenSurface();

				string
					vlcPluginsPath =
					Config::getInstance().getString("VideoPlayerPluginsPath", "");
				//printf("screen->w = %d screen->h = %d screen->format->BitsPerPixel = %d\n",screen->w,screen->h,screen->format->BitsPerPixel);
				::Shared::Graphics::VideoPlayer player(&Renderer::getInstance(),
					introVideoFile,
					introVideoFileFallback,
					window,
					0, 0,
					screen->w,
					screen->h,
					screen->format->BitsPerPixel,
					false,
					vlcPluginsPath,
					SystemFlags::VERBOSE_MODE_ENABLED);
				player.PlayVideo();
				exitAfterIntroVideo = true;
				return;
			}

			SystemFlags::OutputDebug(SystemFlags::debugSystem,
				"In [%s::%s Line: %d]\n", __FILE__,
				__FUNCTION__, __LINE__);
		}

		Intro::~Intro() {
			deleteValues(texts.begin(), texts.end());

			//deleteValues(introTextureList.begin(),introTextureList.end());
			//      if(test) {
			//              glmDelete(test);
			//      }

			//Shared::Graphics::md5::cleanupMD5OpenGL();
		}

		void
			Intro::update() {
			if (exitAfterIntroVideo == true) {
				mouseUpLeft(0, 0);
				//cleanup();
				return;
			}
			timer++;
			if (timer > introTime * GameConstants::updateFps / 1000) {
				SystemFlags::OutputDebug(SystemFlags::debugSystem,
					"In [%s::%s Line: %d]\n", __FILE__,
					__FUNCTION__, __LINE__);

				cleanup();
				return;
			}

			if (Config::getInstance().getBool("AutoTest")) {
				SystemFlags::OutputDebug(SystemFlags::debugSystem,
					"In [%s::%s Line: %d]\n", __FILE__,
					__FUNCTION__, __LINE__);

				AutoTest::getInstance().updateIntro(program);
				return;
			}

			mouse2d = (mouse2d + 1) % Renderer::maxMouse2dAnim;

			if (targetCamera != NULL) {
				t +=
					((0.01f +
					(1.f - t) / 10.f) / 20.f) * (60.f / GameConstants::updateFps);

				//interpolate position
				camera.setPosition(lastCamera.
					getPosition().lerp(t,
						targetCamera->
						getPosition()));

				//interpolate orientation
				Quaternion
					q =
					lastCamera.getOrientation().lerp(t,
						targetCamera->getOrientation());
				camera.setOrientation(q);

				if (t >= 1.f) {
					targetCamera = NULL;
					t = 0.f;
				}
			}

			//fade
			if (fade <= 1.f) {
				fade += 0.6f / GameConstants::updateFps;
				if (fade > 1.f) {
					fade = 1.f;
				}
			}

			//animation
			//const float minSpeed = 0.015f;
			//const float minSpeed = 0.010f;
			//const float maxSpeed = 0.6f;
			const float
				minSpeed = modelMinAnimSpeed;
			const float
				maxSpeed = modelMaxAnimSpeed;
			anim +=
				(maxSpeed / GameConstants::updateFps) / 5 +
				random.randRange(minSpeed,
					max(minSpeed + 0.0001f,
					(maxSpeed / GameConstants::updateFps) / 5.f));
			if (anim > 1.f) {
				anim = 0.f;
			}

			//animTimer.update();
		}

		void
			Intro::renderModelBackground() {
			// Black background
			glClearColor(0, 0, 0, 1);

			if (models.empty() == false) {
				int
					difTime = 1000 * timer / GameConstants::updateFps - modelShowTime;
				int
					totalModelShowTime = Intro::introTime - modelShowTime;
				int
					individualModelShowTime = totalModelShowTime / (int) models.size();

				//printf("difTime = %d individualModelShowTime = %d modelIndex = %d\n",difTime,individualModelShowTime,modelIndex);

				//int difTime= 1;
				if (difTime > 0) {
					if (difTime > ((modelIndex + 1) * individualModelShowTime)) {
						//int oldmodelIndex = modelIndex;
						if (modelIndex + 1 < (int) models.size()) {
							modelIndex++;

							//position
							//nextCamera.setPosition(camera.getPosition());
							//                                      nextCamera.setPosition(Vec3f(84,-9,11));
							//
							//                                      //rotation
							//                                      //Vec3f startRotation(0,12,0);
							//                                      Vec3f startRotation(0,-80,0);
							//                                      nextCamera.setOrientation(Quaternion(EulerAngles(
							//                                              degToRad(startRotation.x),
							//                                              degToRad(startRotation.y),
							//                                              degToRad(startRotation.z))));
							//
							//                                      this->targetCamera = &nextCamera;
							//                                      this->lastCamera= camera;
							//                                      this->t= 0.f;

						}
						//printf("oldmodelIndex = %d, modelIndex = %d\n",oldmodelIndex,modelIndex);
					}
					Renderer & renderer = Renderer::getInstance();
					vector < Model * >characterModels;
					characterModels.push_back(NULL);
					characterModels.push_back(NULL);
					characterModels.push_back(models[modelIndex]);
					const
						Vec3f
						characterPosition = startPosition;
					renderer.renderMenuBackground(&camera, fade, NULL, characterModels,
						characterPosition, anim);
				}
			}
		}

		void
			Intro::render() {
			Renderer & renderer = Renderer::getInstance();
			if (GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
				return;
			}

			canRender();
			incrementFps();

			renderer.clearBuffers();
			renderer.reset3dMenu();

			renderer.clearZBuffer();
			renderer.loadCameraMatrix(&camera);

			renderModelBackground();
			renderer.renderParticleManager(rsMenu);

			renderer.reset2d();

			for (int i = 0; i < (int) texts.size(); ++i) {
				IntroText *
					text = texts[i];

				int
					difTime =
					1000 * timer / GameConstants::updateFps - text->getTime();

				if (difTime > 0 && difTime < appearTime + showTime + disapearTime) {
					float
						alpha = 1.f;
					if (difTime > 0 && difTime < appearTime) {
						//apearing
						alpha = static_cast <float>(difTime) /
							appearTime;
					} else if (difTime > 0
						&& difTime < appearTime + showTime + disapearTime) {
						//disappearing
						alpha =
							1.f - static_cast <float>(difTime - appearTime - showTime) /
							disapearTime;
					}

					if (text->getText().empty() == false) {
						int
							renderX = text->getPos().x;
						int
							renderY = text->getPos().y;

						if (Renderer::renderText3DEnabled) {
							if (renderX < 0) {
								const
									Metrics &
									metrics = Metrics::getInstance();
								int
									w = metrics.getVirtualW();
								renderX =
									(w / 2) -
									(text->getFont3D()->
										getMetrics()->getTextWidth(text->getText()) / 2);
							}
							if (renderY < 0) {
								const
									Metrics &
									metrics = Metrics::getInstance();
								int
									h = metrics.getVirtualH();
								renderY =
									(h / 2) +
									(text->getFont3D()->
										getMetrics()->getHeight(text->getText()) / 2);
							}

							renderer.renderText3D(text->getText(), text->getFont3D(),
								alpha, renderX, renderY, false);
						} else {
							if (renderX < 0) {
								const
									Metrics &
									metrics = Metrics::getInstance();
								int
									w = metrics.getVirtualW();
								renderX = (w / 2);
							}
							if (renderY < 0) {
								const
									Metrics &
									metrics = Metrics::getInstance();
								int
									h = metrics.getVirtualH();
								renderY = (h / 2);
							}

							renderer.renderText(text->getText(), text->getFont(), alpha,
								renderX, renderY, true);
						}
					}

					if (text->getTexture() != NULL) {
						Vec4f color = Vec4f(1.f, 1.f, 1.f, alpha);
						renderer.renderTextureQuad(text->getPos().x, text->getPos().y,
							text->getSize().x,
							text->getSize().y,
							text->getTexture(), &color);
					}
				}
			}

			if (program != NULL)
				program->renderProgramMsgBox();

			if (this->forceMouseRender == true)
				renderer.renderMouse2d(mouseX, mouseY, mouse2d, 0.f);

			bool
				showIntroTiming =
				Config::getInstance().getBool("ShowIntroTiming", "false");
			if (showIntroTiming == true && Intro::introTime > 0) {
				CoreData & coreData = CoreData::getInstance();
				int
					difTime = 1000 * timer / GameConstants::updateFps;
				string
					timingText =
					intToStr(difTime) + " / " + intToStr(Intro::introTime);

				if (Renderer::renderText3DEnabled) {
					//const Metrics &metrics= Metrics::getInstance();
					//int w= metrics.getVirtualW();
					//int h= metrics.getVirtualH();

					renderer.renderText3D(timingText, coreData.getMenuFontVeryBig3D(),
						1, 10, 20, false);
				} else {
					//const Metrics &metrics= Metrics::getInstance();
					//int w= metrics.getVirtualW();
					//int h= metrics.getVirtualH();

					renderer.renderText(timingText, coreData.getMenuFontVeryBig(), 1,
						10, 20, false);
				}
			}

			renderer.renderFPSWhenEnabled(lastFps);
			renderer.swapBuffers();
		}

		void
			Intro::keyDown(SDL_KeyboardEvent key) {
			SDL_keysym keystate = key.keysym;
			//printf("keystate.mod = %d key = unicode[%d] regular[%d] lalt [%d] ralt [%d] alt [%d]\n",keystate.mod,key.keysym.unicode,key.keysym.sym,(keystate.mod & KMOD_LALT),(keystate.mod & KMOD_RALT),(keystate.mod & KMOD_ALT));

			if (keystate.mod & (KMOD_LALT | KMOD_RALT)) {
				//printf("ALT KEY #1\n");

				if (isKeyPressed(SDLK_RETURN, key) == true ||
					isKeyPressed(SDLK_RALT, key) == true ||
					isKeyPressed(SDLK_LALT, key) == true) {
					return;
				}
			}

			//printf("Exiting intro\n");
			mouseUpLeft(0, 0);
		}

		void
			Intro::mouseUpLeft(int x, int y) {
			SoundRenderer & soundRenderer = SoundRenderer::getInstance();

			SystemFlags::OutputDebug(SystemFlags::debugSystem,
				"In [%s::%s Line: %d]\n", __FILE__,
				__FUNCTION__, __LINE__);
			soundRenderer.stopMusic(CoreData::getInstance().getIntroMusic());

			SystemFlags::OutputDebug(SystemFlags::debugSystem,
				"In [%s::%s Line: %d]\n", __FILE__,
				__FUNCTION__, __LINE__);

			if (CoreData::getInstance().hasMainMenuVideoFilename() == false) {
				soundRenderer.playMusic(CoreData::getInstance().getMenuMusic());
			}

			SystemFlags::OutputDebug(SystemFlags::debugSystem,
				"In [%s::%s Line: %d]\n", __FILE__,
				__FUNCTION__, __LINE__);

			cleanup();
		}

		void
			Intro::cleanup() {
			Renderer::getInstance().endMenu();

			program->setState(new MainMenu(program));
		}

		void
			Intro::mouseMove(int x, int y, const MouseState * ms) {
			mouseX = x;
			mouseY = y;
		}

	}
}                              //end namespace
