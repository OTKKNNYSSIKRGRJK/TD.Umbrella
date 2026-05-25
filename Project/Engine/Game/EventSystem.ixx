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
	export template<typename _EventType>
	using EventCallbackFunction = std::function<void(_EventType&)>;

	export class EventInterface {
	public:
		using Handle = I32;

	public:
		EventInterface() noexcept = default;
		virtual ~EventInterface() noexcept = default;
	};

	template<typename _T>
	constexpr bool IsEvent = std::is_base_of_v<EventInterface, _T>;

	class EventHandlerInterface {
	private:
		virtual void Callback(EventInterface&) = 0;

	public:
		inline void NotifyListener(
			EventInterface& event_
		) { Callback(event_); }

	public:
		virtual ~EventHandlerInterface() noexcept = default;
	};

	export template<typename _EventType>
	class EventHandler : public EventHandlerInterface {
	private:
		EventCallbackFunction<_EventType> CallbackFunction{ nullptr };

		virtual void Callback(EventInterface& event_) override {
			CallbackFunction(static_cast<_EventType&>(event_));
		}

	public:
		EventHandler(
			EventCallbackFunction<_EventType> callbackFunc_
		) noexcept : CallbackFunction{ callbackFunc_ } {}
		virtual ~EventHandler() noexcept = default;
	};

	export class EventManager : public NonCopyable<EventManager> {
	public:
		static constexpr int MaxNum_EventType{ 512 };

	private:
		//std::unordered_map<EventInterface::Handle, std::vector<std::unique_ptr<EventHandlerInterface>>> Container_EventListener{};
		//std::queue<std::pair<EventInterface::Handle, std::unique_ptr<EventInterface>>> Container_Event{};

		std::vector<std::vector<std::unique_ptr<EventHandlerInterface>>> Container_EventListeners{ MaxNum_EventType };
		std::queue<std::pair<EventInterface::Handle, std::unique_ptr<EventInterface>>> Container_Event{};

		template<typename EventType>
		void TriggerEventWithTID(EventType&& event_, EventInterface::Handle tid_) {
			auto& eventListeners{ Container_EventListeners.at(tid_) };
			for (auto&& it{ eventListeners.cbegin() }; it != eventListeners.cend(); ++it) {
				(*it)->NotifyListener(event_);
			}
		}

	public:
		void Initialize();
		void Finalize();

		template<typename _EventType>
		void AddEventListener(EventCallbackFunction<_EventType>&& handler_) {
			static_assert(IsEvent<_EventType>);

			std::unique_ptr<EventHandlerInterface> eventHandler{
				std::make_unique<EventHandler<_EventType>>(std::move(handler_))
			};

			std::string_view name{ typeid(_EventType).name() };
			EventInterface::Handle eventTID{ EventTypes[name] };
			auto& eventListeners{ Container_EventListeners.at(eventTID) };
			eventListeners.emplace_back(std::move(eventHandler));
		}

		template<typename _EventType>
		void RemoveEventListener(EventCallbackFunction<_EventType> const& callbackFunc_) {
			static_assert(IsEvent<_EventType>);
		}

		template<typename _EventType>
		inline void TriggerEvent(_EventType&& event_) {
			static_assert(IsEvent<_EventType>);

			std::string_view name{ typeid(_EventType).name() };
			EventInterface::Handle eventTID{ EventTypes[name] };
			TriggerEventWithTID(event_, eventTID);
		}

		template<typename _EventType>
		void QueueEvent(_EventType&& event_) {
			static_assert(IsEvent<_EventType>);

			std::unique_ptr<EventInterface> newEvent{ std::make_unique<_EventType>(event_) };
			std::string_view name{ typeid(_EventType).name() };
			EventInterface::Handle eventTID{ EventTypes[name] };
			Container_Event.push(std::make_pair(eventTID, newEvent));
		}

		void DispatchEvent();

	private:
		EventInterface::Handle Count_EventTypeHandle{ 0 };
		std::unordered_map<std::string_view, EventInterface::Handle> EventTypes{};

	public:
		// Returns a new Handle if the SceneType is not yet registered, or returns the Handle of the SceneType.
		template<typename _EventType>
		constexpr EventInterface::Handle RegisterType() {
			try {
				if (Count_EventTypeHandle > MaxNum_EventType) {
					throw "Cannot register any new EventType.\n";
				}
			}
			catch ([[maybe_unused]] const char* errorMsg) {}

			std::string_view name{ typeid(_EventType).name() };
			if (!EventTypes.contains(typeid(_EventType).name())) {
				EventInterface::Handle newHandle{ Count_EventTypeHandle };
				EventTypes.emplace(std::make_pair(name, newHandle));
				++Count_EventTypeHandle;
			}

			return EventTypes[name];
		}
	};
}