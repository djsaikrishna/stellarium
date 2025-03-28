/*
 * Stellarium Telescope Control Plug-in
 *
 * Copyright (C) 2006 Johannes Gajdosik
 * Copyright (C) 2009-2010 Bogdan Marinov
 *
 * This module was originally written by Johannes Gajdosik in 2006
 * as a core module of Stellarium. In 2009 it was significantly extended with
 * GUI features and later split as an external plug-in module by Bogdan Marinov.
 *
 * This class used to be called TelescopeMgr before the split.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#ifndef TELESCOPECONTROL_HPP
#define TELESCOPECONTROL_HPP

#include "StelFader.hpp"
#include "StelGui.hpp"
#include "StelObjectModule.hpp"
#include "StelProjectorType.hpp"
#include "StelTextureTypes.hpp"
#include "VecMath.hpp"

#include <QFile>
#include <QFont>
#include <QHash>
#include <QMap>
#include <QProcess>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QVariant>

class StelObject;
class StelPainter;
class StelProjector;
class TelescopeClient;
class TelescopeDialog;
class SlewDialog;

typedef QSharedPointer<TelescopeClient> TelescopeClientP;

/*! @defgroup telescopeControl Telescope Control plug-in
@{
The Telescope Control plug-in allows Stellarium to control
a telescope on a computerized mount (a "Go To" or "Push To"
telescope) and offers a graphical user interface for
setting up the connection.
@}
*/

//! @class TelescopeControl
//! @ingroup telescopeControl
//! Main class of the Telescope Control plug-in.

//! This class manages the controlling of one or more telescopes by one
//! instance of the stellarium program. "Controlling a telescope"
//! means receiving position information from the telescope
//! and sending GOTO commands to the telescope.
//! No esoteric features like motor focus, electric heating and such.
//! The actual controlling of a telescope is left to the implementation
//! of the abstract base class TelescopeClient.
class TelescopeControl : public StelObjectModule
{
	Q_OBJECT
	Q_PROPERTY(bool flagTelescopeReticles              READ getFlagTelescopeReticles           WRITE setFlagTelescopeReticles          NOTIFY flagTelescopeReticlesChanged)
	Q_PROPERTY(bool flagTelescopeLabels                READ getFlagTelescopeLabels             WRITE setFlagTelescopeLabels            NOTIFY flagTelescopeLabelsChanged)
	Q_PROPERTY(bool flagTelescopeCircles               READ getFlagTelescopeCircles            WRITE setFlagTelescopeCircles           NOTIFY flagTelescopeCirclesChanged)
	Q_PROPERTY(Vec3f reticleColor                      READ getReticleColor                    WRITE setReticleColor                   NOTIFY reticleColorChanged)
	Q_PROPERTY(Vec3f labelColor                        READ getLabelColor                      WRITE setLabelColor                     NOTIFY labelColorChanged)
	Q_PROPERTY(Vec3f circleColor                       READ getCircleColor                     WRITE setCircleColor                    NOTIFY circleColorChanged)
	Q_PROPERTY(bool useTelescopeServerLogs             READ getFlagUseTelescopeServerLogs      WRITE setFlagUseTelescopeServerLogs     NOTIFY flagUseTelescopeServerLogsChanged)
	Q_PROPERTY(bool useTelescopeServerExecutables      READ getFlagUseServerExecutables        WRITE setFlagUseServerExecutables       NOTIFY getFlagUseServerExecutablesChanged)
	Q_PROPERTY(QString serverExecutablesDirectoryPath  READ getServerExecutablesDirectoryPath  WRITE setServerExecutablesDirectoryPath NOTIFY serverExecutablesDirectoryPathChanged)

public:
	enum ConnectionType {
		ConnectionNA = 0,
		ConnectionVirtual,
		ConnectionInternal,
		ConnectionLocal,
		ConnectionRemote,
		ConnectionRTS2,
		ConnectionINDI,
		ConnectionASCOM,
		ConnectionTypeCount
	};
	Q_ENUM(ConnectionType)

	enum TelescopeStatus {
		StatusNA = 0,
		StatusStarting,
		StatusConnecting,
		StatusConnected,
		StatusDisconnected,
		StatusStopped
		//StatusCount
	};
	Q_ENUM(TelescopeStatus)

	enum Equinox {
		EquinoxJ2000,
		EquinoxJNow
	};
	Q_ENUM(Equinox)

	struct DeviceModel
	{
		QString name;
		QString description;
		QString server;
		int defaultDelay;
		bool useExecutable;
	};

	TelescopeControl();
	~TelescopeControl() override;

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	void init() override;
	void deinit() override;
	void update(double deltaTime) override;
	void draw(StelCore* core) override;
	double getCallOrder(StelModuleActionName actionName) const override;

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelObjectModule class
	QList<StelObjectP> searchAround(const Vec3d& v, double limitFov, const StelCore* core) const override;
	StelObjectP searchByNameI18n(const QString& nameI18n) const override;
	StelObjectP searchByName(const QString& name) const override;
	StelObjectP searchByID(const QString& id) const override { return searchByName(id); }
	//! Find and return the list of at most maxNbItem objects auto-completing the passed object name.
	//! @param objPrefix the case insensitive first letters of the searched object
	//! @param maxNbItem the maximum number of returned object names
	//! @param useStartOfWords the autofill mode for returned objects names
	//! @return a list of matching object name by order of relevance, or an empty list if nothing match
	QStringList listMatchingObjects(const QString& objPrefix, const int maxNbItem = 5, bool useStartOfWords = false) const override;
	// empty as its not celestial objects
	QStringList listAllObjects(bool) const override { return QStringList(); }
	QString getName() const override { return "Telescope Control"; }
	QString getStelObjectType() const override;
	bool configureGui(bool show = true) override;

	QSharedPointer<TelescopeClient> telescopeClient(int index) const;

	//! Remove all currently registered telescopes
	void deleteAllTelescopes();

	//! Safe access to the loaded list of telescope models
	const QHash<QString, DeviceModel> &getDeviceModels();

	//! Loads the module's configuration from the configuration file.
	void loadConfiguration();
	//! Saves the module's configuration to the configuration file.
	void saveConfiguration();

	//! Saves to telescopes.json a list of the parameters of the active telescope clients.
	void saveTelescopes();
	//! Loads from telescopes.json the parameters of telescope clients and initializes them. If there are
	//! already any initialized telescope clients, they are removed.
	void loadTelescopes();

	// These are public, but not slots, because they don't use sufficient validation. Scripts shouldn't be
	// able to add/remove telescopes, only to point them.
	//! Adds a telescope description containing the given properties. DOES NOT VALIDATE its parameters. If
	//! serverName is specified, portSerial should be specified too. Call saveTelescopes() to write the
	//! modified configuration to disc. Call startTelescopeAtSlot() to start this telescope.
	//! @param portSerial must be a valid serial port name for the particular platform, e.g. "COM1" for
	//! Microsoft Windows of "/dev/ttyS0" for Linux
	bool addTelescopeAtSlot(int slot, ConnectionType connectionType, QString name, QString equinox,
				QString host = QString("localhost"), int portTCP = DEFAULT_TCP_PORT, int delay = DEFAULT_DELAY,
				bool connectAtStartup = false, QList<double> circles = QList<double>(), QString serverName = QString(),
				QString portSerial = QString(), QString rts2Url = QString(), QString rts2Username = QString(),
				QString rts2Password = QString(), int rts2Refresh = -1, QString ascomDeviceId = QString(""),
				bool ascomUseDeviceEqCoordType = true);
	//! Retrieves a telescope description. Returns false if the slot is empty. Returns empty serverName and
	//! portSerial if the description contains no server.
	bool getTelescopeAtSlot(int slot, ConnectionType& connectionType, QString& name, QString& equinox,
				QString& host, int& portTCP, int& delay, bool& connectAtStartup, QList<double>& circles,
				QString& serverName, QString& portSerial, QString& rts2Url, QString& rts2Username,
				QString& rts2Password, int& rts2Refresh, QString& ascomDeviceId, bool& ascomUseDeviceEqCoordType);
	//! Removes info from the tree. Should it include stopTelescopeAtSlot()?
	bool removeTelescopeAtSlot(int slot);

	//! Starts a telescope at the given slot, getting its description with getTelescopeAtSlot(). Creates a
	//! TelescopeClient object and starts a server process if necessary.
	bool startTelescopeAtSlot(int slot);
	//! Stops the telescope at the given slot. Destroys the TelescopeClient object and terminates the server
	//! process if necessary.
	bool stopTelescopeAtSlot(int slot);
	//! Stops all telescopes, but without removing them like deleteAllTelescopes().
	bool stopAllTelescopes();

	//! Checks if there's a TelescopeClient object at a given slot, i.e. if there's an active telescope at
	//! that slot.
	bool isExistingClientAtSlot(int slot);
	//! Checks if the TelescopeClient object at a given slot is connected to a server.
	bool isConnectedClientAtSlot(int slot);

	//! Returns a list of the currently connected clients
	QHash<int, QString> getConnectedClientsNames();

	bool getFlagUseServerExecutables() const { return useServerExecutables; }
	//! Forces a call of loadDeviceModels(). Stops all active telescopes.
	void setFlagUseServerExecutables(bool b);
	const QString& getServerExecutablesDirectoryPath() const;
	//! Forces a call of loadDeviceModels(). Stops all active telescopes.
	bool setServerExecutablesDirectoryPath(const QString& newPath);

	bool getFlagUseTelescopeServerLogs() const { return useTelescopeServerLogs; }

	static constexpr int MIN_SLOT_NUMBER = 1;
	static constexpr int SLOT_COUNT = 9;
	static constexpr int SLOT_NUMBER_LIMIT = MIN_SLOT_NUMBER + SLOT_COUNT;
	static constexpr int MAX_SLOT_NUMBER = SLOT_NUMBER_LIMIT - 1;

	static constexpr int BASE_TCP_PORT = 10000;
	#define DEFAULT_TCP_PORT_FOR_SLOT(X) (TelescopeControl::BASE_TCP_PORT + X)
	static constexpr int DEFAULT_TCP_PORT = DEFAULT_TCP_PORT_FOR_SLOT(MIN_SLOT_NUMBER);

	static constexpr int MAX_CIRCLE_COUNT = 10;
	static const QString TELESCOPE_SERVER_PATH;
	static constexpr int DEFAULT_DELAY = 500000; //Microseconds; == 0.5 seconds
	#define MICROSECONDS_FROM_SECONDS(X) (X * 1000000)
	#define SECONDS_FROM_MICROSECONDS(X) (static_cast<double>(X) / 1000000)
	static const QStringList EMBEDDED_TELESCOPE_SERVERS;

public slots:
	//! Set display flag for telescope reticles
	//! @param b boolean flag
	//! @code
	//! // example of usage in scripts
	//! TelescopeControl.setFlagTelescopeReticles(true);
	//! @endcode
	void setFlagTelescopeReticles(bool b) { reticleFader = b; emit flagTelescopeReticlesChanged(b);}
	//! Get display flag for telescope reticles
	//! @return true if telescope reticles is visible
	//! @code
	//! // example of usage in scripts
	//! var flag = TelescopeControl.getFlagTelescopeReticles();
	//! @endcode
	bool getFlagTelescopeReticles() const { return static_cast<bool>(reticleFader); }

	//! Set display flag for telescope name labels
	//! @param b boolean flag
	//! @code
	//! // example of usage in scripts
	//! TelescopeControl.setFlagTelescopeLabels(true);
	//! @endcode
	void setFlagTelescopeLabels(bool b) { labelFader = b; emit flagTelescopeLabelsChanged(b);}
	//! Get display flag for telescope name labels
	//! @return true if telescope name labels is visible
	//! @code
	//! // example of usage in scripts
	//! var flag = TelescopeControl.getFlagTelescopeLabels();
	//! @endcode
	bool getFlagTelescopeLabels() const { return labelFader == true; }

	//! Set display flag for telescope field of view circles
	//! @param b boolean flag
	//! @code
	//! // example of usage in scripts
	//! TelescopeControl.setFlagTelescopeCircles(true);
	//! @endcode
	void setFlagTelescopeCircles(bool b) { circleFader = b; emit flagTelescopeCirclesChanged(b);}
	//! Get display flag for telescope field of view circles
	//! @return true if telescope field of view circles is visible
	//! @code
	//! // example of usage in scripts
	//! var flag = TelescopeControl.getFlagTelescopeCircles();
	//! @endcode
	bool getFlagTelescopeCircles() const { return circleFader == true; }

	//! Set the telescope reticle color
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(1.0, 0.0, 0.0);
	//! TelescopeControl.setReticleColor(c.toVec3f());
	//! @endcode
	void setReticleColor(const Vec3f& c) { reticleColor = c; emit reticleColorChanged(c);}
	//! Get the telescope reticle color
	//! @return the telescope reticle color
	//! @code
	//! // example of usage in scripts
	//! color = TelescopeControl.getReticleColor();
	//! @endcode
	const Vec3f& getReticleColor() const { return reticleColor; }

	//! Get the telescope labels color
	//! @return the telescope labels color
	//! @code
	//! // example of usage in scripts
	//! color = TelescopeControl.getLabelColor();
	//! @endcode
	const Vec3f& getLabelColor() const { return labelColor; }
	//! Set the telescope labels color
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(1.0, 0.0, 0.0);
	//! TelescopeControl.setLabelColor(c.toVec3f());
	//! @endcode
	void setLabelColor(const Vec3f& c) { labelColor = c; emit labelColorChanged(c);}

	//! Set the field of view circles color
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(1.0, 0.0, 0.0);
	//! TelescopeControl.setCircleColor(c.toVec3f());
	//! @endcode
	void setCircleColor(const Vec3f& c) { circleColor = c; emit circleColorChanged(c);}
	//! Get the field of view circles color
	//! @return the field of view circles color
	//! @code
	//! // example of usage in scripts
	//! color = TelescopeControl.getCircleColor();
	//! @endcode
	const Vec3f& getCircleColor() const { return circleColor; }

	//! Define font size to use for telescope names display
	//! @param fontSize size of font
	//! @code
	//! // example of usage in scripts
	//! TelescopeControl.setFontSize(15);
	//! @endcode
	void setFontSize(int fontSize);

	//! slews a telescope at slot idx to the selected object.
	//! @code
	//! // example of usage in scripts
	//! TelescopeControl.slewTelescopeToSelectedObject(1);
	//! @endcode
	void slewTelescopeToSelectedObject(const int idx);

	//! sync a telescope at slot idx to the selected object. The telescope client can decide what syncing
	//! means but usually its used to update the internal pointing model.
	//! @code
	//! // example of usage in scripts
	//! TelescopeControl.syncTelescopeToSelectedObject(1);
	//! @endcode
	void syncTelescopeWithSelectedObject(const int idx);

	//! slews a telescope at slot idx to the point of the celestial sphere currently
	//! in the center of the screen.
	//! @code
	//! // example of usage in scripts
	//! TelescopeControl.slewTelescopeToViewDirection(1);
	//! @endcode
	void slewTelescopeToViewDirection(const int idx);

	//! abort the current slew command of a telescope at slot idx.
	//! @note ATTENTION! Not all telescopes support this call! A warning panel may be shown instead. Then it's jump and run to prevent damage.
	//! @code
	//! // example of usage in scripts
	//! TelescopeControl.abortTelescopeSlew(1);
	//! @endcode
	void abortTelescopeSlew(const int idx);

	//! Centering screen by coordinates of a telescope at slot idx.
	//! @code
	//! // example of usage in scripts
	//! TelescopeControl.centeringScreenByTelescope(1);
	//! @endcode
	void centeringScreenByTelescope(const int idx);

	//! Used in the GUI
	void setFlagUseTelescopeServerLogs(bool b) { useTelescopeServerLogs = b; emit flagUseTelescopeServerLogsChanged(b); }

signals:
	void clientConnected(int slot, QString name);
	void clientDisconnected(int slot);

	void flagTelescopeReticlesChanged(bool b);
	void flagTelescopeLabelsChanged(bool b);
	void flagTelescopeCirclesChanged(bool b);
	void reticleColorChanged(const Vec3f &c);
	void labelColorChanged(const Vec3f &c);
	void circleColorChanged(const Vec3f &c);
	void flagUseTelescopeServerLogsChanged(bool b);
	void useTelescopeServerExecutables(bool b);
	void getFlagUseServerExecutablesChanged(bool b);
	void serverExecutablesDirectoryPathChanged(const QString &s);

private slots:
	//! Set translated keyboard shortcut descriptions.
	void translateActionDescriptions();

private:
	///////////////////////////////////////////////////////////////////////////
	// Methods specific to TelescopeControl
	//! Send a J2000-goto-command to the specified telescope
	//! @param telescopeNr the number of the telescope
	//! @param j2000Pos the direction in equatorial J2000 frame
	//! @param selectObject selected object (if any; nullptr if move is not based on an object)
	void telescopeGoto(int telescopeNr, const Vec3d& j2000Pos, StelObjectP selectObject = nullptr);

	void telescopeSync(int telescopeNr, const Vec3d &j2000Pos, StelObjectP selectObject = nullptr);

	//! Draw a nice animated pointer around the object if it's selected
	void drawPointer(const StelProjectorP& prj, const StelCore* core, StelPainter& sPainter);

	//! Perform the communication with the telescope servers
	void communicate(void);

	LinearFader labelFader;
	LinearFader reticleFader;
	LinearFader circleFader;
	//! Colour currently used to draw telescope reticles
	Vec3f reticleColor;
	//! Colour currently used to draw telescope text labels
	Vec3f labelColor;
	//! Colour currently used to draw field of view circles
	Vec3f circleColor;

	//! Font used to draw telescope text labels
	QFont labelFont;

	// Toolbar button to toggle the Slew window
	StelButton* toolbarButton;

	//! Telescope reticle texture
	StelTextureSP reticleTexture;
	//! Telescope selection marker texture
	StelTextureSP selectionTexture;

	//! Contains the initialized telescope client objects representing the telescopes that Stellarium is
	//! connected to or attempting to connect to.
	QMap<int, TelescopeClientP> telescopeClients;
	//! Contains QProcess objects of the currently running telescope server processes that have been launched
	//! by Stellarium.
	QHash<int, QProcess*> telescopeServerProcess;
	QStringList telescopeServers;
	QVariantMap telescopeDescriptions;
	QHash<QString, DeviceModel> deviceModels;

	QHash<ConnectionType, QString> connectionTypeNames;

	bool useTelescopeServerLogs;
	QHash<int, QFile*> telescopeServerLogFiles;
	QHash<int, QTextStream*> telescopeServerLogStreams;

	bool useServerExecutables;
	QString serverExecutablesDirectoryPath;

	// GUI
	TelescopeDialog* telescopeDialog;
	SlewDialog* slewDialog;

	//! Used internally. Checks if the argument is a valid slot number.
	static bool isValidSlotNumber(int slot);
	static bool isValidPort(uint port);
	static bool isValidDelay(int delay);

	//! Start the telescope server defined for a given slot in a new QProcess
	//! @param slot the slot number
	//! @param serverName the short form of the server name (e.g. "Dummy" for "TelescopeServerDummy")
	//! @param tcpPort TCP slot the server should listen to
	bool startServerAtSlot(int slot, QString serverName, int tcpPort, QString serialPort);
	//! Stop the telescope server at a given slot, terminating the process
	bool stopServerAtSlot(int slot);

	//! A wrapper for TelescopeClient::create(). Used internally by loadTelescopes() and
	//! startTelescopeAtSlot(). Does not perform any validation on its arguments.
	bool startClientAtSlot(int slot, ConnectionType connectionType, QString name, QString equinox,
			       QString host, int portTCP, int delay, QList<double> circles, QString serverName = QString(),
			       QString portSerial = QString(), QString rts2Url = QString(), QString rts2Username = QString(),
			       QString rts2Password = QString(), int rts2Refresh = -1, QString ascomDeviceId = QString(""),
			       bool ascomUseDeviceEqCoordType = true);

	//! Returns true if the TelescopeClient at this slot has been stopped successfully or doesn't exist
	bool stopClientAtSlot(int slot);

	//! Compile a list of the executables in the /servers folder
	void loadTelescopeServerExecutables();

	//! Loads the list of supported telescope models. Calls loadTelescopeServerExecutables() internally.
	void loadDeviceModels();

	//! Copies the default device_models.json to the given destination
	//! @returns true if the file has been copied successfully
	bool restoreDeviceModelsListTo(QString deviceModelsListPath);

	void addLogAtSlot(int slot);
	void logAtSlot(int slot);
	void removeLogAtSlot(int slot);

	static void translations();

	QString actionGroupId;
	QString moveToSelectedActionId;
	QString syncActionId;
	QString abortSlewActionId;
	QString moveToCenterActionId;
	QString centeringScreenActionId;
};

#include "StelPluginInterface.hpp"
#include <QObject>

//! This class is used by Qt to manage a plug-in interface
class TelescopeControlStelPluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID StelPluginInterface_iid)
	Q_INTERFACES(StelPluginInterface)
public:
	StelModule* getStelModule() const override;
	StelPluginInfo getPluginInfo() const override;
	//QObjectList getExtensionList() const override { return QObjectList(); }
};

#endif /* TELESCOPECONTROL_HPP */
