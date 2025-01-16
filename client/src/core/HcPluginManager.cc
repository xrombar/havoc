#include <Havoc.h>
#include <core/HcPluginManager.h>
#include <Python.h>

class HcCoreApp : public IHcApplication {

public:
    ~HcCoreApp() override = default;

    auto StyleSheet(
        void
    ) -> QString override {
        return Havoc->StyleSheet();
    }

    auto MainWindowWidget(
        void
    ) -> QMainWindow* override {
        return Havoc->ui;
    }

    auto PageAgentAddTab(
        const std::string& name,
        const QIcon&       icon,
        QWidget*           widget
    ) -> void override {
        Havoc->ui->PageAgent->addTab(
            QString::fromStdString( name ),
            icon,
            widget
        );
    }

    auto RegisterAgentAction(
        const std::string&           action_name,
        const QIcon&                 action_icon,
        HcFnCallbackCtx<std::string> action_func,
        const std::string&           agent_type
    ) -> void override {
        const auto action = new HcApplication::ActionObject;

        action->type       = HcApplication::ActionObject::ActionAgent;
        action->name       = action_name;
        action->icon       = action_icon;
        action->callback   = reinterpret_cast<HcFnCallbackCtx<void*>>( action_func );
        action->agent.type = agent_type;

        Havoc->AddAction( action );
    }

    auto RegisterAgentAction(
        const std::string&           action_name,
        const QIcon&                 action_icon,
        HcFnCallbackCtx<std::string> action_func
    ) -> void override {
        const auto action = new HcApplication::ActionObject;

        action->type     = HcApplication::ActionObject::ActionAgent;
        action->name     = action_name;
        action->icon     = action_icon;
        action->callback = reinterpret_cast<HcFnCallbackCtx<void*>>( action_func );

        Havoc->AddAction( action );
    }

    auto RegisterAgentAction(
        const std::string&           action_name,
        HcFnCallbackCtx<std::string> action_func,
        bool                         multi_select
    ) -> void override {
        //
        // TODO: implement
        //
    }

    auto RegisterMenuAction(
        const std::string& action_name,
        const QIcon&       action_icon,
        HcFnCallback       action_func
    ) -> void override {
        const auto action = new HcApplication::ActionObject;

        action->type     = HcApplication::ActionObject::ActionHavoc;
        action->name     = action_name;
        action->icon     = action_icon;
        action->callback = reinterpret_cast<HcFnCallbackCtx<void*>>( action_func );

        Havoc->AddAction( action );
    };

    auto Agent(
        const std::string& uuid
    ) -> std::optional<IHcAgent *> override {
        return Havoc->Agent( uuid );
    }

    auto PythonContextRun(
        std::function<void()> function,
        const bool            concurrent
    ) -> void override {
        //
        // invoke the function in the current thread
        // if concurrent is not enabled or specified
        if ( !concurrent ) {
            HcPythonAcquire();
            function();

            return;
        }

        //
        // start the python context run in a separate thread
        //
        // TODO: this sometimes wont get triggered properly
        //       look into this or change this to QThread
        const auto future = QtConcurrent::run( []( const std::function<void()>& Fn ) {
            HcPythonAcquire();
            Fn();
        }, function );

        QApplication::processEvents();
    }
};

HcPluginManager::HcPluginManager() : core_app( new HcCoreApp ) {}

auto HcPluginManager::loadPlugin(
    const std::string& path
) -> void {

    spdlog::debug( "path: {}", path );
    auto loader = QPluginLoader( QString::fromStdString( path ) );
    
    
    spdlog::debug( "loader: {}\n loader.instance(): {}", loader, loader.instance );
    auto plugin = qobject_cast<IHcPlugin*>( loader.instance() );


    spdlog::debug( "loader.instance(): {} ({}) factory: {}",
        fmt::ptr( loader.instance() ),
        loader.instance()->metaObject()->className(),
        fmt::ptr( plugin )
    );

    spdlog::debug( "hot potato 3" );
    if ( !plugin ) {
        spdlog::error(
            "HcPluginManager::loadPlugin failed to load plugin {}: {}",
            path, loader.errorString().toStdString()
        );
        return;
    }

    spdlog::debug(
        "HcPluginManager::loadPlugin loaded {} @ {}",
        loader.metaData().value( "IID" ).toString().toStdString(),
        fmt::ptr( plugin )
    );

    spdlog::debug( "invoking entrypoint of plugin" );

    plugin->main( core_app );

    spdlog::debug( "adding plugin to the list" );

    plugins.push_back( plugin );
}
