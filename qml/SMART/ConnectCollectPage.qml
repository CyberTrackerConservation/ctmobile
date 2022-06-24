import QtQml 2.12
import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

C.ContentPage {
    id: page

    property string selectedCountry
    property var servers: []
    property var selectedServer
    property var packages
    property var selectedPackage

    header: C.PageHeader {
        text: qsTr("Select region")
    }

    C.CountryListModel {
        id: countryListModel
        Component.onCompleted: {
            insert(0, { name: qsTr("All regions"), iso2: "", iso3: "" })
        }
    }

    C.ListViewV {
        anchors.fill: parent        
        model: countryListModel
        delegate: ItemDelegate {
            width: ListView.view.width
            contentItem: RowLayout {
                Image {
                    source: iso2 !== "" ? "qrc:/countryflags/" + iso2.toLowerCase() + ".svg" : ""
                    sourceSize.width: 48
                }

                Label {
                    Layout.fillWidth: true
                    text: name
                    font.pixelSize: App.settings.font16
                }

                C.ChevronRight {}
            }

            C.HorizontalDivider {}

            onClicked: {
                page.selectedCountry = iso3
                busyCover.doWork = getServers
                busyCover.start()
            }
        }
    }

    Component {
        id: serverPage

        C.ContentPage {
            header: C.PageHeader {
                text: qsTr("Select server")
            }

            C.ListViewV {
                anchors.fill: parent
                model: page.servers
                delegate: C.ListRowDelegate {
                    width: ListView.view.width
                    text: modelData.name
                    subText: modelData.server.split("/")[2].split(":")[0]
                    onClicked: {
                        page.selectedServer = modelData
                        busyCover.doWork = getPackages
                        busyCover.start()
                    }
                    chevronRight: true
                }
            }
        }
    }

    Component {
        id: packagePage

        C.ContentPage {
            header: C.PageHeader {
                text: qsTr("Select package")
            }

            C.ListViewV {
                anchors.fill: parent
                model: page.packages
                delegate: C.ListRowDelegate {
                    width: ListView.view.width
                    text: modelData.name
                    subText: App.formatDateTime(modelData.uploadedDate)
                    onClicked: {
                        page.selectedPackage = modelData
                        busyCover.doWork = connectPackage
                        busyCover.start()
                    }
                }
            }
        }
    }

    function getServers() {
        page.servers = []
        let response = Utils.httpGet("https://connectlocations.smartconservationtools.org/SMARTConnect/connectservers.json")

        switch (response.status) {
        case 200:
            let servers = []
            try {
                servers = JSON.parse(response.data)
            } catch (error) {
                servers = [
                    {
                      "server": "https://connect7.refractions.net:8443/server/noa/smartcollect/packages",
                      "name": qsTr("Test Server"),
                      "countrylist": ""
                    }
                ]
            }

            for (let i = 0; i < servers.length; i++) {
                let server = servers[i];
                if (page.selectedCountry === "" || server.countrylist === "") {
                    page.servers.push(server)
                    continue
                }

                let countryList = server.countrylist.split(",")
                if (countryList.indexOf(page.selectedCountry) !== -1) {
                    page.servers.push(server)
                    continue
                }
            }

            if (page.servers.length > 0) {
                page.servers.sort(sortCompareByName)
                appPageStack.push(serverPage)
            } else {
                showError(qsTr("No servers found"))
            }
            break

        default:
            showError(qsTr("Connection error"))
            break
        }
    }

    function getPackages() {
        let response = Utils.httpGet(page.selectedServer.server)

        switch (response.status) {
        case 200:
            page.packages = JSON.parse(response.data)
            if (page.packages.length > 0) {
                page.packages.sort(sortCompareByName)
                appPageStack.push(packagePage)
            } else {
                showError(qsTr("No packages found"))
            }
            break

        default:
            showError(qsTr("Connection error"))
            break
        }
    }

    function connectPackage() {
        let params = {
            server: page.selectedServer.server + "/" + page.selectedPackage.uuid
        }

        App.projectManager.bootstrap("SMART", params)
        App.projectManager.projectsChanged()

        postPopToRoot()
    }
}
