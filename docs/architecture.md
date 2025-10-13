
# KIVA OS Architecture

KIVA OS is built on a **decoupled, event-driven architecture** to promote scalability, maintainability, and efficiency. This design avoids common pitfalls like "God Objects" and tight coupling, resulting in a more robust and flexible system.

## Core Concepts

The architecture is centered around three key concepts:

1.  **Event Dispatcher (`EventDispatcher`):** A central singleton that acts as an "event bus." It allows different parts of the system (Services, UI components) to communicate with each other by publishing and subscribing to events. This eliminates direct dependencies; for example, a menu doesn't need to know about the `Jammer` service to start it. It simply publishes a `JAMMER_START_REQUESTED` event.

2.  **Services (`Service`):** Long-running tasks and hardware managers (e.g., `WifiManager`, `Jammer`, `MusicPlayer`) are encapsulated as `Service` classes. These services are managed by the `ServiceManager` and interact with the rest of the system by listening for and publishing events. They are not polled in the main loop unless they are active, which improves efficiency.

3.  **UI State Machine (`App`):** The main `App` class is responsible for managing the UI navigation stack. It acts as the primary subscriber to navigation events (e.g., `NAVIGATE_TO_MENU`, `NAVIGATE_BACK`) and holds the registry of all `IMenu` instances. It is no longer a monolithic object that controls everything, but a dedicated manager for the user interface.

## Event Flow Example

Here's a typical event flow for starting the WiFi jammer:

1.  The user presses the "OK" button on the `JammingMenu`.
2.  The `JammingMenu`, which is subscribed to `INPUT` events, receives the button press.
3.  Instead of calling a `jammer->start()` method directly, it publishes a `JAMMER_START_REQUESTED` event to the `EventDispatcher`.
4.  The `Jammer` service, which is subscribed to `JAMMER_START_REQUESTED`, receives the event.
5.  The `Jammer` service then starts the jamming process and might publish a `JAMMER_STARTED` event to notify other parts of the system (like the UI) that the state has changed.

## Benefits of this Architecture

-   **Decoupling:** Components don't need direct knowledge of each other.
-   **Scalability:** Adding new features is as simple as creating a new service and defining the events it subscribes and publishes to.
-   **Efficiency:** The main loop is clean, and services are not polled unnecessarily.
-   **Testability:** Individual components can be tested in isolation by sending them events and observing their responses.
