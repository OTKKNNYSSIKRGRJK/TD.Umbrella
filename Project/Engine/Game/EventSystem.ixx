//////	//////	//////	//////	//////	//////	//////	//////	//////
//	Reference:														//
//	https://denyskryvytskyi.github.io/event-system					//
//////	//////	//////	//////	//////	//////	//////	//////	//////

export module Lumina.EventSystem;

import <unordered_map>;
import <vector>;
import <queue>;
import <string_view>;
import <functional>;
import <memory>;

import Lumina.Core.Common;

namespace Lumina {
	template<typename EventType>
	using EventCallbackFunction = std::function<void(const EventType&)>;

	class EventInterface {
	public:
		using Handle = int;

		EventInterface() = default;
		virtual ~EventInterface() = default;
	};

	template<typename T>
	constexpr bool IsEvent = std::is_base_of_v<EventInterface, T>;

	class EventHandlerInterface {
	private:
		virtual void Callback(const EventInterface&) = 0;

	public:
		virtual ~EventHandlerInterface() = default;

		void Notify_Listener(const EventInterface& e) {
			Callback(e);
		}
	};

	template<typename EventType>
	class EventHandler : public EventHandlerInterface {
	private:
		EventCallbackFunction<EventType> CallbackFunction{ nullptr };

		virtual void Callback(const EventInterface& event) override {
			CallbackFunction(static_cast<const EventType&>(event));
		}

	public:
		explicit EventHandler(std::function<void(const EventType&)> callbackFunc) : CallbackFunction(callbackFunc) {}
		virtual ~EventHandler() = default;
	};

	class EventManager : public NonCopyable<EventManager> {
	public:
		static constexpr int MaxNum_EventType{ 512 };

	private:
		//std::unordered_map<EventInterface::Handle, std::vector<std::unique_ptr<EventHandlerInterface>>> Container_EventListener{};
		//std::queue<std::pair<EventInterface::Handle, std::unique_ptr<EventInterface>>> Container_Event{};

		std::vector<std::vector<std::unique_ptr<EventHandlerInterface>>> Container_EventListeners{ MaxNum_EventType };
		std::queue<std::pair<EventInterface::Handle, std::unique_ptr<EventInterface>>> Container_Event{};

		template<typename EventType>
		void Trigger_Event_with_TID(const EventType& _evt, const EventInterface::Handle _tid) {
			auto& eventListeners{ Container_EventListeners.at(_tid) };
			for (auto&& it{ eventListeners.cbegin() }; it != eventListeners.cend(); ++it) {
				(*it)->Notify_Listener(_evt);
			}
		}

	public:
		void Initialize();
		void Finalize();

		template<typename EventType>
		void Add_EventListener(const EventHandler<EventType>& _handler) {
			static_assert(IsEvent<EventType>);

			std::unique_ptr<EventHandlerInterface> eventHandler{ std::make_unique<EventHandler<EventType>>(_handler) };

			EventInterface::Handle eventTID{ EventType::TypeHandle };
			auto& eventListeners{ Container_EventListeners.at(eventTID) };
			eventListeners.emplace_back(eventHandler);
		}

		template<typename EventType>
		void Remove_EventListener(const EventCallbackFunction<EventType>& callbackFunc) {
			static_assert(IsEvent<EventType>);
		}

		template<typename EventType>
		inline void Trigger_Event(const EventType& _evt) {
			static_assert(IsEvent<EventType>);

			Trigger_Event_with_TID(_evt, EventType::TypeHandle);
		}

		template<typename EventType>
		void Queue_Event(const EventType& evt) {
			static_assert(IsEvent<EventType>);

			std::unique_ptr<EventInterface> newEvent{ std::make_unique<EventType>(evt) };
			Container_Event.push(std::make_pair(EventType::Get_TID(), newEvent));
		}

		void Dispatch_Event();

	private:
		EventInterface::Handle Count_EventTypeHandle{ 0 };
		std::unordered_map<std::string_view, EventInterface::Handle> EventTypes{};

	public:
		// Returns a new Handle if the SceneType is not yet registered, or returns the Handle of the SceneType.
		template<typename EventType>
		constexpr EventInterface::Handle Register_Type() {
			try {
				if (Count_EventTypeHandle > MaxNum_EventType) {
					throw "Cannot register any new EventType.\n";
				}
			}
			catch ([[maybe_unused]] const char* errorMsg) {}

			if (!EventTypes.contains(EventType::TypeName)) {
				EventInterface::Handle newHandle{ Count_EventTypeHandle };
				EventTypes.emplace(std::make_pair(EventType::TypeName, newHandle));
				++Count_EventTypeHandle;
			}

			return EventTypes[EventType::TypeName];
		}
	};


	#define LUMINA_EVENT(T)\
	class T : public Lumina::EventInterface

	#define LUMINA_REGEISTER_EVENTTYPE(T)\
	public:\
		static constexpr std::string_view TypeName{ #T };\
		static inline const Lumina::EventInterface::Handle TypeHandle{ Lumina::EventManager::Instance()->Register_Type<T>() }

	#define ADD_EVENTLISTENER(_eventType, _callback) Lumina::EventManager::Instance()->Add_EventListener<_eventType>(EventHandler<_eventType>(_callback))
}