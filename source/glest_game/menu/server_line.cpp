//
//      server_line.cpp:
//
//      This file is part of ZetaGlest <https://github.com/ZetaGlest>
//
//      Copyright (C) 2018  The ZetaGlest team
//
//      ZetaGlest is a fork of MegaGlest <https://megaglest.org>
//
//      This program is free software: you can redistribute it and/or modify
//      it under the terms of the GNU General Public License as published by
//      the Free Software Foundation, either version 3 of the License, or
//      (at your option) any later version.
//
//      This program is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU General Public License for more details.
//
//      You should have received a copy of the GNU General Public License
//      along with this program.  If not, see <https://www.gnu.org/licenses/>```


#include "server_line.h"

#include "renderer.h"
#include "core_data.h"
#include "config.h"
#include "metrics.h"
#include "auto_test.h"
#include "masterserver_info.h"
#include "versions.h"

#include "leak_dumper.h"

namespace Glest {
	namespace Game {

		// =====================================================
		//      class ServerLine
		// =====================================================

		ServerLine::ServerLine(MasterServerInfo * mServerInfo, int lineIndex,
			int baseY, int lineHeight,
			const char *containerName) {
			this->containerName = containerName;
			this->countryTexture = NULL;
			Lang & lang = Lang::getInstance();

			this->lineHeight = lineHeight;
			int lineOffset = lineHeight * lineIndex;
			masterServerInfo = *mServerInfo;
			int i = 5;
			this->baseY = baseY;
			Vec4f color = Vec4f(1.0f, 1.0f, 1.0f, 1.0f);

			if (masterServerInfo.getConnectedClients() == 0) {
				color = Vec4f(0.6f, 0.7f, 1.0f, 1.0f);
			}


			//general info:
			//i+= 10;
			glestVersionLabel.init(i, baseY - lineOffset);
			glestVersionLabel.setRenderBackground(true);
			glestVersionLabel.setMaxEditRenderWidth(970);  // use background for whole line
			glestVersionLabel.setTextColor(color);
			glestVersionLabel.setText(" " + masterServerInfo.getGlestVersion());
			glestVersionLabel.setFont(CoreData::getInstance().
				getDisplayFontSmall());
			glestVersionLabel.setFont3D(CoreData::getInstance().
				getDisplayFontSmall3D());

			i += 80;
			string platform = masterServerInfo.getPlatform();
			size_t revOffset = platform.find("-Rev");
			if (revOffset != platform.npos) {
				platform = platform.substr(0, revOffset);
			}

			platformLabel.init(i, baseY - lineOffset);
			platformLabel.setTextColor(color);
			platformLabel.setText(platform);
			platformLabel.setFont(CoreData::getInstance().getDisplayFontSmall());
			platformLabel.setFont3D(CoreData::getInstance().
				getDisplayFontSmall3D());

			//      i+=50;
			//      registeredObjNameList.push_back("binaryCompileDateLabel" + intToStr(lineIndex));
			//      binaryCompileDateLabel.registerGraphicComponent(containerName,"binaryCompileDateLabel" + intToStr(lineIndex));
			//      binaryCompileDateLabel.init(i,baseY-lineOffset);
			//      binaryCompileDateLabel.setText(masterServerInfo.getBinaryCompileDate());

			//game info:
			i += 120;
			serverTitleLabel.init(i, baseY - lineOffset);
			serverTitleLabel.setTextColor(color);
			serverTitleLabel.setText(masterServerInfo.getServerTitle());

			i += 170;
			country.init(i, baseY - lineOffset);
			country.setTextColor(color);
			country.setText(masterServerInfo.getCountry());

			string data_path =
				getGameReadWritePath(GameConstants::path_data_CacheLookupKey);
			string countryLogoPath = data_path + "data/core/misc_textures/flags";

			Config & config = Config::getInstance();
			if (config.getString("CountryTexturePath", "") != "") {
				countryLogoPath = config.getString("CountryTexturePath", "");
			}
			endPathWithSlash(countryLogoPath);

			string logoFile =
				countryLogoPath + toLower(masterServerInfo.getCountry()) + ".png";
			if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).
				enabled)
				SystemFlags::OutputDebug(SystemFlags::debugSystem,
					"In [%s::%s Line: %d] logoFile [%s]\n",
					__FILE__, __FUNCTION__, __LINE__,
					logoFile.c_str());

			if (fileExists(logoFile) == true) {
				countryTexture =
					GraphicsInterface::getInstance().getFactory()->newTexture2D();
				//loadingTexture = renderer.newTexture2D(rsGlobal);
				countryTexture->setMipmap(true);
				//loadingTexture->getPixmap()->load(filepath);
				if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).
					enabled)
					SystemFlags::OutputDebug(SystemFlags::debugSystem,
						"In [%s::%s Line: %d] logoFile [%s]\n",
						__FILE__, __FUNCTION__, __LINE__,
						logoFile.c_str());
				countryTexture->load(logoFile);

				if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).
					enabled)
					SystemFlags::OutputDebug(SystemFlags::debugSystem,
						"In [%s::%s Line: %d]\n", __FILE__,
						__FUNCTION__, __LINE__);

				Renderer & renderer = Renderer::getInstance();
				renderer.initTexture(rsGlobal, countryTexture);
			}

			i += 60;
			//      ipAddressLabel.init(i,baseY-lineOffset);
			//      ipAddressLabel.setText(masterServerInfo.getIpAddress());
			//      i+=100;

			wrongVersionLabel.init(i, baseY - lineOffset);
			wrongVersionLabel.setTextColor(Vec4f(1.0f, 0.0f, 0.0f, 1.0f));
			wrongVersionLabel.setText(lang.getString("IncompatibleVersion"));

			//game setup info:
			techLabel.init(i, baseY - lineOffset);
			techLabel.setTextColor(color);
			techLabel.setText(masterServerInfo.getTech());

			i += 130;
			mapLabel.init(i, baseY - lineOffset);
			mapLabel.setTextColor(color);
			mapLabel.setText(masterServerInfo.getMap());
			i += 130;

			//      tilesetLabel.init(i,baseY-lineOffset);
			//      tilesetLabel.setText(masterServerInfo.getTileset());
			//      i+=100;

			activeSlotsLabel.init(i, baseY - lineOffset);
			activeSlotsLabel.setTextColor(color);
			activeSlotsLabel.
				setText(intToStr(masterServerInfo.getActiveSlots()) + "/" +
					intToStr(masterServerInfo.getNetworkSlots()) + "/" +
					intToStr(masterServerInfo.getConnectedClients()));

			i += 50;
			//externalConnectPort.init(i, baseY - lineOffset);
			//externalConnectPort.setTextColor(color);
			//externalConnectPort.setText(intToStr(masterServerInfo.getExternalConnectPort()));

			i += 30;
			status.init(i - 10, baseY - lineOffset);
			status.setTextColor(color);
			status.setText(lang.
				getString("MGGameStatus" +
					intToStr(masterServerInfo.getStatus())));

			i += 130;
			selectButton.init(i + 25, baseY - lineOffset, 35, lineHeight - 5);
			selectButton.setText(">");
			selectButton.setAlwaysLighted(true);

			//printf("glestVersionString [%s] masterServerInfo->getGlestVersion() [%s]\n",glestVersionString.c_str(),masterServerInfo->getGlestVersion().c_str());
			compatible =
				checkVersionComptability(GAME_VERSION,
					masterServerInfo.getGlestVersion());
			selectButton.setEnabled(compatible);
			selectButton.setEditable(compatible);

		}

		void ServerLine::reloadUI() {
			Lang & lang = Lang::getInstance();

			glestVersionLabel.setText(masterServerInfo.getGlestVersion());

			string platform = masterServerInfo.getPlatform();
			size_t revOffset = platform.find("-Rev");
			if (revOffset != platform.npos) {
				platform = platform.substr(0, revOffset);
			}

			platformLabel.setText(platform);

			serverTitleLabel.setText(masterServerInfo.getServerTitle());

			country.setText(masterServerInfo.getCountry());

			wrongVersionLabel.setText(lang.getString("IncompatibleVersion"));

			techLabel.setText(masterServerInfo.getTech());

			mapLabel.setText(masterServerInfo.getMap());
			activeSlotsLabel.
				setText(intToStr(masterServerInfo.getActiveSlots()) + "/" +
					intToStr(masterServerInfo.getNetworkSlots()) + "/" +
					intToStr(masterServerInfo.getConnectedClients()));

			//externalConnectPort.setText(intToStr(masterServerInfo.getExternalConnectPort()));

			status.setText(lang.
				getString("MGGameStatus" +
					intToStr(masterServerInfo.getStatus())));

			GraphicComponent::
				reloadFontsForRegisterGraphicComponents(containerName);
		}

		ServerLine::~ServerLine() {
			//delete masterServerInfo;

			if (countryTexture != NULL) {
				if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).
					enabled)
					SystemFlags::OutputDebug(SystemFlags::debugSystem,
						"In [%s::%s Line: %d]\n", __FILE__,
						__FUNCTION__, __LINE__);

				countryTexture->end();
				delete countryTexture;

				if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).
					enabled)
					SystemFlags::OutputDebug(SystemFlags::debugSystem,
						"In [%s::%s Line: %d]\n", __FILE__,
						__FUNCTION__, __LINE__);

				//delete loadingTexture;
				countryTexture = NULL;
			}
		}

		bool ServerLine::buttonMouseClick(int x, int y) {
			return selectButton.mouseClick(x, y);
		}

		bool ServerLine::buttonMouseMove(int x, int y) {
			return selectButton.mouseMove(x, y);
		}

		void ServerLine::render() {
			Renderer & renderer = Renderer::getInstance();
			//general info:
			renderer.renderLabel(&glestVersionLabel);
			renderer.renderLabel(&platformLabel);
			//renderer.renderLabel(&binaryCompileDateLabel);


			//game info:
			renderer.renderLabel(&serverTitleLabel);
			if (countryTexture != NULL) {
				Vec4f color = Vec4f(1.f, 1.f, 1.f, 0.7f);
				renderer.renderTextureQuad(country.getX() + 1, country.getY() + 4,
					countryTexture->getTextureWidth(),
					countryTexture->getTextureHeight(),
					countryTexture, &color);
			} else {
				renderer.renderLabel(&country);
			}
			if (compatible) {
				if (selectButton.getEnabled() == true) {
					//renderer.renderLabel(&ipAddressLabel);
					//game setup info:
					renderer.renderLabel(&techLabel);
					renderer.renderLabel(&mapLabel);
					//renderer.renderLabel(&tilesetLabel);
					renderer.renderLabel(&activeSlotsLabel);
					//renderer.renderLabel(&externalConnectPort);
					renderer.renderLabel(&status);
				}
			} else {
				renderer.renderLabel(&wrongVersionLabel);
			}
			renderer.renderLabel(&status);

			bool joinEnabled =
				(masterServerInfo.getNetworkSlots() >
					masterServerInfo.getConnectedClients());
			if (joinEnabled == true) {
				if (compatible) {
					selectButton.setEnabled(true);
					selectButton.setVisible(true);
					renderer.renderButton(&selectButton);
				}
			} else {
				selectButton.setEnabled(false);
				selectButton.setVisible(false);
			}
		}

		void ServerLine::setY(int y) {
			selectButton.setY(y);

			//general info:
			glestVersionLabel.setY(y);
			platformLabel.setY(y);
			//binaryCompileDateLabel.setY(y);

			//game info:
			serverTitleLabel.setY(y);
			country.setY(y);
			status.setY(y);
			//ipAddressLabel.setY(y);

			//game setup info:
			techLabel.setY(y);
			mapLabel.setY(y);
			//tilesetLabel.setY(y);
			activeSlotsLabel.setY(y);

			//externalConnectPort.setY(y);

		}

	}
}                               //end namespace
