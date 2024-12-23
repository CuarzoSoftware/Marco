#include <Marco/MApplication.h>
#include <Marco/MScreen.h>
#include <Marco/roles/MToplevel.h>
#include <AK/nodes/AKContainer.h>
#include <AK/nodes/AKSimpleText.h>
#include <iostream>

using namespace Marco;
using namespace AK;

int main()
{
    MApplication app;

    app.on.screenPlugged.subscribe(&app, [](MScreen &screen){
        std::cout << "New Screen! " << screen.props().name << std::endl;
    });

    app.on.screenUnplugged.subscribe(&app, [](MScreen &screen){
        std::cout << "Bye bye Screen! " << screen.props().name << std::endl;
    });

    MToplevel window;
    window.layout().setAlignItems(YGAlignCenter);
    window.layout().setJustifyContent(YGJustifyCenter);

    AKSimpleText helloWorld { "Hello World!", &window };
    window.show();

    return app.exec();
}
