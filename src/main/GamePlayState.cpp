/******************************************************************************
 *
 *    This file is part of openDarkEngine project
 *    Copyright (C) 2005-2006 openDarkEngine team
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *	  $Id$
 *
 *****************************************************************************/

#include "OIS.h"
#include "GameStateManager.h"
#include "GamePlayState.h"
#include "GameLoadState.h"
#include "logger.h"
#include "integers.h"


#include <OgreRenderWindow.h>
#include <OgreOverlayElement.h>
#include <OgreStringConverter.h>

using namespace Ogre;
using namespace OIS;

namespace Opde {

	template<> GamePlayState* Singleton<GamePlayState>::ms_Singleton = 0;

	GamePlayState::GamePlayState() : mSceneMgr(NULL), mDebugOverlay(NULL), mToLoadScreen(true) {
	    /// Register as a command listener, so we can load different levels
	  Opde::ConsoleBackend::getSingleton().registerCommandListener("load", dynamic_cast<ConsoleCommandListener*>(this));
		Opde::ConsoleBackend::getSingleton().setCommandHint("load", "Loads a specified mission file");
		Opde::ConsoleBackend::getSingleton().registerCommandListener("fps", dynamic_cast<ConsoleCommandListener*>(this));
		Opde::ConsoleBackend::getSingleton().setCommandHint("fps", "Dump FPS stats");

		mRotateSpeed = 36;
		mMoveSpeed = 50;
		mRotateYFactor = 1;

		mShadows = false;

        // Try to remap the parameters with those listed in the configuration
		mConfigService = ServiceManager::getSingleton().getService("ConfigService").as<ConfigService>();
        GameStateManager* gsm = GameStateManager::getSingletonPtr();


        if (mConfigService->hasParam("move_speed"))
					mMoveSpeed = mConfigService->getParam("move_speed").toFloat();

        if (mConfigService->hasParam("mouse_speed"))
          mRotateSpeed = mConfigService->getParam("mouse_speed").toFloat();

        if (mConfigService->hasParam("mouse_invert"))
          mRotateYFactor = mConfigService->getParam("mouse_invert").toFloat();

		mTranslateVector = Vector3::ZERO;
		mRotX = 0;
		mRotY = 0;

		mForward = false;
		mBackward = false;
		mLeft = false;
		mRight = false;
		mScreenShot = false;

		mSceneDetailIndex = 0;
		mNumScreenShots = 1;

		mRoot = Root::getSingletonPtr();
		mOverlayMgr = OverlayManager::getSingletonPtr();

		mConsole = new ConsoleFrontend();
        mConsole->setActive(false);

		mDebugOverlay = OverlayManager::getSingleton().getByName("Opde/DebugOverlay");

		// Portal stats overlay
		mPortalOverlay = OverlayManager::getSingleton().getByName("Opde/OpdeDebugOverlay");
		
		mShadows = true;
	}

    GamePlayState::~GamePlayState() {
        delete mConsole;
    }

	void GamePlayState::start() {
    LOG_INFO("GamePlayState: Starting");
		PropertyGroupPtr PropertyGroup = ServiceManager::getSingleton().getService("PropertyService").
				as<PropertyService>()->getPropertyGroup("Position");
		
		if (PropertyGroup.isNull())
			OPDE_EXCEPT("Could not get Position property group. Not defined. Fatal", "GamePlayState::start");
		
		LOG_DEBUG("Starting Point object id : %d", StartingPointObjID);	
		
		DVariant spoint;
		PropertyGroup->get(StartingPointObjID, "position", spoint);
		
		Vector3 StartingPoint(0,0,0);
		
		if (spoint.type() == DVariant::DV_VECTOR)
			StartingPoint = spoint.toVector();
		
		LOG_DEBUG("Starting Point position : %f %f %f", StartingPoint.x, StartingPoint.y, StartingPoint.z);	
		
//		std::string tmp = PropertyGroup->get(StartingPointObjID, "SymName").toString();
		mSceneMgr = mRoot->getSceneManager( "DarkSceneManager" );
		RenderServicePtr renderSrv = ServiceManager::getSingleton().getService("RenderService").as<RenderService>();
		
		mCamera = renderSrv->getDefaultCamera();
		mViewport = renderSrv->getDefaultViewport();
		mWindow = renderSrv->getRenderWindow();

		mSceneMgr->clearSpecialCaseRenderQueues();
		mSceneMgr->setSpecialCaseRenderQueueMode(SceneManager::SCRQM_EXCLUDE);

		mCamera->setNearClipDistance(0.5);
		mCamera->setFarClipDistance(4000);

		// Also change position, and set Quake-type orientation
		ViewPoint vp = mSceneMgr->getSuggestedViewpoint(true);
		mCamera->setPosition(vp.position);
		
		if (StartingPointObjID != 0)
			mCamera->setPosition(StartingPoint);
			
		mCamera->pitch(Degree(90));
		mCamera->rotate(vp.orientation);

		// Don't yaw along variable axis, causes leaning
		mCamera->setFixedYawAxis(true, Vector3::UNIT_Z);


		// Thiefy FOV
		mCamera->setFOVy(Degree(70));

        if (mConfigService->hasParam("debug")) {
            if (mConfigService->getParam("debug") == true) {
                // debug overlay
                mDebugOverlay->show();

                // Portal stats overlay
                mPortalOverlay->show();

                mDebug = true;
            }
        }

        // hidden as default
		mConsole->setActive(false);

		mWindow->resetStatistics();

        mToLoadScreen = false;
        
// Set up a debug panel to display the shadow
		MaterialPtr debugMat = MaterialManager::getSingleton().create(
				"Ogre/DebugShadowMap0", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
		debugMat->getTechnique(0)->getPass(0)->setLightingEnabled(false);
		TexturePtr shadowTex = mSceneMgr->getShadowTexture(0);
		TextureUnitState *t = debugMat->getTechnique(0)->getPass(0)->createTextureUnitState(shadowTex->getName());
		t->setTextureAddressingMode(TextureUnitState::TAM_CLAMP);

/*		debugMat = MaterialManager::getSingleton().create(
				"Ogre/DebugShadowMap1", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
		debugMat->getTechnique(0)->getPass(0)->setLightingEnabled(false);
		shadowTex = mSceneMgr->getShadowTexture(1);
		t = debugMat->getTechnique(0)->getPass(0)->createTextureUnitState(shadowTex->getName());
		t->setTextureAddressingMode(TextureUnitState::TAM_CLAMP);
*/
		// Uncomment this to display the shadow textures
		OverlayContainer* debugPanel = (OverlayContainer*)
				(OverlayManager::getSingleton().createOverlayElement("Panel", "Ogre/DebugShadowPanel0"));
		debugPanel->_setPosition(0.8, 0);
		debugPanel->_setDimensions(0.2, 0.2);
		debugPanel->setMaterialName("Ogre/DebugShadowMap0");
		Overlay* debugOverlay = OverlayManager::getSingleton().getByName("Opde/DebugOverlay");
		debugOverlay->add2D(debugPanel);

/*		debugPanel = (OverlayContainer*)
				(OverlayManager::getSingleton().createOverlayElement("Panel", "Ogre/DebugShadowPanel1"));
		debugPanel->_setPosition(0.8, 0.2);
		debugPanel->_setDimensions(0.2, 0.2);
		debugPanel->setMaterialName("Ogre/DebugShadowMap1");
		debugOverlay->add2D(debugPanel);
*/

		LOG_INFO("GamePlayState: Started");
	}

	void GamePlayState::exit() {
	    LOG_INFO("GamePlayState: Exiting");

	    // mConsole->setActive(false);

		// clear the scene
		mSceneMgr->clearScene();

		mPortalOverlay->hide();
		mDebugOverlay->hide();
		mConsole->setActive(false);
		mConsole->update(1); // to hide the console while exiting

		if (mToLoadScreen) {
            pushState(GameLoadState::getSingletonPtr());
            mToLoadScreen = false;
        }

		LOG_INFO("GamePlayState: Exited");
	}

	void GamePlayState::suspend() {
	    LOG_INFO("GamePlayState: Suspend?!");
	    mToLoadScreen = false;
	}

	void GamePlayState::resume() {
	    LOG_INFO("GamePlayState: Resume?!");
   		mToLoadScreen = false;
	}

	void GamePlayState::update(unsigned long timePassed) {
		if (timePassed == 0) {
			mMoveScale = 0.1f;
			mRotScale = 0.1f;
		} else {
			mMoveScale = mMoveSpeed * timePassed / 1000;
			mRotScale = mRotateSpeed * timePassed / 1000;
		}

		// Quick hack. Let the camera move:
		if (mForward)
			mTranslateVector.z = -mMoveScale;

		if (mBackward)
			mTranslateVector.z =  mMoveScale;

		if (mLeft)
			mTranslateVector.x = -mMoveScale;

		if (mRight)
			mTranslateVector.x =  mMoveScale;

		mCamera->yaw(mRotX * mRotScale);
		mCamera->pitch(mRotY * mRotScale);
		mCamera->moveRelative(mTranslateVector);

		mTranslateVector = Vector3::ZERO;
		mRotX = 0;
		mRotY = 0;

		if (mSceneDisplay) {
			mSceneDetailIndex = (mSceneDetailIndex+1)%2 ; // I Do not need points for now
			switch(mSceneDetailIndex) {
				case 0 : mCamera->setPolygonMode(PM_SOLID) ; break ;
				case 1 : mCamera->setPolygonMode(PM_WIREFRAME) ; break ;
				//case 2 : mCamera->setPolygonMode(PM_POINTS) ; break ;
			}
			mSceneDisplay = false;
		}

		if (mPortalDisplay) {
			// reuse
			mSceneMgr->getOption("ShowPortals", &mPortalDisplay);
			mPortalDisplay = !mPortalDisplay;
			mSceneMgr->setOption("ShowPortals", &mPortalDisplay);

			mPortalDisplay = false;
		}

		if (mScreenShot) {
            char tmp[20];
            sprintf(tmp, "screenshot_%d.png", ++mNumScreenShots);
            RenderWindow* w = Ogre::Root::getSingleton().getAutoCreatedWindow();

            w->writeContentsToFile(tmp);

		    mScreenShot = false;
		}
		
		if (mSMWriteToFile) {
			// write Ogre/DebugShadowMap0 to file
			TexturePtr TextureToSave =  mSceneMgr->getShadowTexture(0);
			
			HardwarePixelBufferSharedPtr readbuffer;
			readbuffer = TextureToSave->getBuffer(0, 0);
			readbuffer->lock(HardwareBuffer::HBL_NORMAL );
			const PixelBox &readrefpb = readbuffer->getCurrentLock();	
			uchar *readrefdata = static_cast<uchar*>(readrefpb.data);		

			Image img;
			img = img.loadDynamicImage (readrefdata, TextureToSave->getWidth(),
				TextureToSave->getHeight(), TextureToSave->getFormat());	
			img.save("smdebug.png");
    
			readbuffer->unlock();
			
			mSMWriteToFile = false;
		}

		mConsole->update(timePassed);

		// Temporary: Debug Overlay
		static String currFps = "Current FPS: ";
		static String avgFps = "Average FPS: ";
		static String bestFps = "Best FPS: ";
		static String worstFps = "Worst FPS: ";
		static String tris = "Triangle Count: ";
		static String batches = "Batch Count: ";

        if (mDebug) {
            // update stats when necessary
            try {
                OverlayElement* guiAvg = OverlayManager::getSingleton().getOverlayElement("Opde/AverageFps");
                OverlayElement* guiCurr = OverlayManager::getSingleton().getOverlayElement("Opde/CurrFps");
                OverlayElement* guiBest = OverlayManager::getSingleton().getOverlayElement("Opde/BestFps");
                OverlayElement* guiWorst = OverlayManager::getSingleton().getOverlayElement("Opde/WorstFps");

                const RenderTarget::FrameStats& stats = mWindow->getStatistics();

                guiAvg->setCaption(avgFps + StringConverter::toString(stats.avgFPS));
                guiCurr->setCaption(currFps + StringConverter::toString(stats.lastFPS));
                guiBest->setCaption(bestFps + StringConverter::toString(stats.bestFPS)
                +" "+StringConverter::toString(stats.bestFrameTime)+" ms");
                guiWorst->setCaption(worstFps + StringConverter::toString(stats.worstFPS)
                +" "+StringConverter::toString(stats.worstFrameTime)+" ms");

                OverlayElement* guiTris = OverlayManager::getSingleton().getOverlayElement("Opde/NumTris");
                guiTris->setCaption(tris + StringConverter::toString(stats.triangleCount));

                OverlayElement* guiBatches = OverlayManager::getSingleton().getOverlayElement("Opde/NumBatches");
                guiBatches->setCaption(batches + StringConverter::toString(stats.batchCount));

                // OverlayElement* guiDbg = OverlayManager::getSingleton().getOverlayElement("Core/DebugText");
            }
            catch(...)
            {
                // ignore
            }

            // update the portal statistics
            try {
                // Volca: I've disabled the timing reports, they need a patch of SM to work
                OverlayElement* guibc = OverlayManager::getSingleton().getOverlayElement("Opde/BackCulls");
                OverlayElement* guiep = OverlayManager::getSingleton().getOverlayElement("Opde/EvalPorts");
                OverlayElement* guirc = OverlayManager::getSingleton().getOverlayElement("Opde/RendCells");
                OverlayElement* guitt = OverlayManager::getSingleton().getOverlayElement("Opde/TravTime");
                OverlayElement* guisr = OverlayManager::getSingleton().getOverlayElement("Opde/StaticRenderTime");

                // Temporary: Debug Overlay
                static String sbc = "Backface culls: ";
                static String sep = "Evaluated portals: ";
                static String src = "Rendered cells: ";
                static String stt = "Traversal Time: ";
                static String ssr = "Static Render Time: ";

                uint bculls, eports, rendc, travtm, statrt;

                mSceneMgr->getOption("BackfaceCulls", &bculls);
                mSceneMgr->getOption("CellsRendered", &rendc);
                mSceneMgr->getOption("EvaluatedPortals", &eports);
                mSceneMgr->getOption("TraversalTime", &travtm);
                mSceneMgr->getOption("StaticRenderTime", &statrt);

                guibc->setCaption(sbc + StringConverter::toString(bculls));
                guiep->setCaption(sep + StringConverter::toString(eports));
                guirc->setCaption(src + StringConverter::toString(rendc));
                guitt->setCaption(stt + StringConverter::toString(travtm) + " ms");
                guisr->setCaption(ssr + StringConverter::toString(statrt) + " ms");
            }
            catch(...)
            {
                // ignore
            }
        }
	}

	bool GamePlayState::keyPressed( const OIS::KeyEvent &e ) {
		if( e.key == KC_F12 ) {
			mConsole->setActive(!mConsole->isActive());
			return true;
		}

    if (!mConsole->injectKeyPress(e)) {
			if(e.key == KC_W) {
				mForward = true;
				return true;
			} else if(e.key == KC_S) {
				mBackward = true;
				return true;
			} else if(e.key == KC_A) {
				mLeft = true;
				return true;
			} else if(e.key == KC_D) {
				mRight = true;
				return true;
			} else if (e.key == KC_SYSRQ) {
				mScreenShot = true;
				return true;
			} else if (e.key == KC_SCROLL) {
				mSMWriteToFile = true;
				return true;
			} else if (e.key == KC_O) {
				mSceneDisplay = true;
				return true;
			} else if (e.key == KC_P) {
				mPortalDisplay = true;
				return true;
			} else return true;
		 } else {
			return true;
		};
	}

	bool GamePlayState::keyReleased( const OIS::KeyEvent &e ) {
		if (!mConsole->isActive()) {
			if(e.key == KC_W) {
				mForward = false;
				return true;
			} else if(e.key == KC_S) {
				mBackward = false;
				return true;
			} else if(e.key == KC_A) {
				mLeft = false;
				return true;
			} else if(e.key == KC_D) {
				mRight = false;
				return true;
			} else	if( e.key == KC_ESCAPE ) {
        			requestTermination();
				return true;
			} else  if (e.key == KC_I) {
				mShadows = !mShadows;
				
				if (mShadows)
					mSceneMgr->setShadowTechnique(SHADOWTYPE_TEXTURE_MODULATIVE);
				else 
					mSceneMgr->setShadowTechnique(SHADOWTYPE_NONE);
			}
			
			return true;
    } else return true;
	}

	bool GamePlayState::mouseMoved( const OIS::MouseEvent &e ) {
		mRotX -= Degree( e.state.X.rel * 20.00);
		// use Y axis invert
		mRotY -= Degree( e.state.Y.rel * 20.00 * mRotateYFactor);
		return false;
	}

	bool GamePlayState::mousePressed( const OIS::MouseEvent &e, OIS::MouseButtonID id ) {
		return false;
	}

	bool GamePlayState::mouseReleased( const OIS::MouseEvent &e, OIS::MouseButtonID id ) {
		return false;
	}

	void GamePlayState::commandExecuted(std::string command, std::string parameters) {
	    std::cerr << "command " << command  << " " << parameters << std::endl;

	    if (command == "load") {
	        // specify the mission file to load by the load state, then switch to the load state
	        mConfigService->setParam("mission", parameters);
            mToLoadScreen = true;
            popState();
	    } else if (command == "fps") {
	            const RenderTarget::FrameStats& stats = mWindow->getStatistics();

                LOG_INFO("Average FPS : %10.2f", stats.avgFPS);
                LOG_INFO("Last FPS    : %10.2f", stats.lastFPS);
                LOG_INFO("Worst FPS   : %10.2f", stats.worstFPS);
	    }
	}

	void GamePlayState::onLinkPlayerFactoryMsg(const LinkChangeMsg& msg) {
		switch (msg.change) {
			case PROP_ADDED   : {
				LOG_INFO("GamePlayState: Found StartingPoint");
        // get the Link ref.
        LinkPtr l = mPlayerFactoryRelation->getLink(msg.linkID);
				StartingPointObjID = l->src();
				break;
			}
		}
	}

	void GamePlayState::bootstrapFinished() {
		mLinkService = ServiceManager::getSingleton().getService("LinkService").as<LinkService>();
		Relation::ListenerPtr metaPropCallback =
			new ClassCallback<LinkChangeMsg, GamePlayState>(this, &GamePlayState::onPropSymNameMsg);

		mPlayerFactoryRelation = mLinkService->getRelation("PlayerFactory");

		if (mPlayerFactoryRelation.isNull())
		    OPDE_EXCEPT("MetaProp relation not found. Fatal.", "InheritService::init");

		mPlayerFactoryListenerID = mPlayerFactoryRelation->registerListener(metaPropCallback);
    LOG_INFO("GamePlayState::bootstrapFinished() - done");
	}

}

