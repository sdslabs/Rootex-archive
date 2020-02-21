#include "common/common.h"

#include "core/audio/audio_source.h"
#include "core/audio/audio_system.h"
#include "core/audio/static_audio_buffer.h"
#include "core/audio/streaming_audio_buffer.h"

#include "core/input/input_manager.h"

#include "core/renderer/buffer_format.h"
#include "core/renderer/index_buffer.h"
#include "core/renderer/material.h"
#include "core/renderer/renderer.h"
#include "core/renderer/shader_library.h"
#include "core/renderer/vertex_buffer.h"

#include "core/resource_loader.h"
#include "core/event_manager.h"

#include "framework/components/visual/visual_component.h"
#include "framework/components/visual/diffuse_visual_component.h"
#include "framework/components/visual/visual_component_graph.h"
#include "framework/components/test_component.h"
#include "framework/components/hierarchy_component.h"
#include "framework/entity_factory.h"
#include "framework/systems/debug_system.h"
#include "framework/systems/test_system.h"
#include "framework/systems/render_system.h"

#include "main/window.h"

#include "os/os.h"
#include "os/thread.h"
#include "os/timer.h"

#include "script/interpreter.h"

int main()
{
	OS::Initialize();
	OS::PrintLine("Rootex Engine is starting: Build(" + OS::GetBuildDate() + "|" + OS::GetBuildTime() + ")");
	DirectX::SimpleMath::Vector2 v2(1.0f, 1.0f);
	OS::PrintLine(std::to_string(v2.Length()));
	AudioSystem::GetSingleton()->initialize();
	AudioResourceFile* w = ResourceLoader::CreateAudioResourceFile("game/assets/lost_in_istanbul.wav");
	Ref<StreamingAudioBuffer> audio(new StreamingAudioBuffer(w));
	Ref<StreamingAudioSource> source(new StreamingAudioSource(audio.get()));
	source->setLooping(true);
	source->play();

	EventHandler* gameEventHandler = new EventHandler([](const Event* event) { WARN(std::get<String>(event->getData())); });
	EventManager::GetSingleton()->addListener(gameEventHandler, Event::Type::Test);
	Ref<Event> testEvent(new Event("Test Event", Event::Type::Test, String("SDSLabs")));
	EventManager::GetSingleton()->call(testEvent);
	EventManager::GetSingleton()->deferredCall(testEvent);

	AudioResourceFile* w2 = ResourceLoader::CreateAudioResourceFile("game/assets/lost_in_istanbul.wav");
	Ref<StaticAudioBuffer> audio2(new StaticAudioBuffer(w2));
	Ref<StaticAudioSource> source2(new StaticAudioSource(audio2.get()));
	source2->setLooping(false);

	TextResourceFile* r = ResourceLoader::CreateTextResourceFile("rootex/test/abc.txt"); // So this loads build/game/abc.txt (However the binary exists in build/game/Debug/)
	OS::PrintLine(r->getString());

	LuaTextResourceFile* windowSettings = ResourceLoader::CreateLuaTextResourceFile("game/assets/config/window.lua");
	OS::PrintLine(windowSettings->getString());

	Vector<Ref<Task>> ju;
	Ref<DebugTask> db(new DebugTask());
	Ref<RenderTask> rb(new RenderTask());

	ju.push_back(db);
	ju.push_back(rb);

	ThreadPool th;
	th.initialize();
	th.submit(ju);
	th.submit(ju);
	th.shutdown();

	LuaInterpreter inter;
	inter.loadExecuteScript(windowSettings);

	LuaVariable windowLua = inter.getGlobal("window");
	Ptr<Window> window(new Window(
	    windowLua["x"],
	    windowLua["y"],
	    windowLua["deltaX"],
	    windowLua["deltaY"],
	    windowLua["title"]));
	InputManager::GetSingleton()->initialize(windowLua["deltaX"], windowLua["deltaY"]);
	// Jump is spacebar
	InputManager::GetSingleton()->mapBool(InputAction::Jump, Device::Keyboard, Key::KeySpace);
	InputManager::GetSingleton()->mapFloat(InputAction::MouseX, Device::Mouse, MouseButton::MouseAxisX);
	InputManager::GetSingleton()->mapFloat(InputAction::MouseY, Device::Mouse, MouseButton::MouseAxisY);

	ShaderLibrary::MakeShaders();

	Ref<VisualComponentGraph> visualGraph(new VisualComponentGraph(windowLua["deltaX"], windowLua["deltaY"]));
	Ref<RenderSystem> renderSystem(new RenderSystem());
	
	LuaTextResourceFile* teapotEntity = ResourceLoader::CreateLuaTextResourceFile("game/assets/test/teapot.lua");
	Ref<Entity> teapot = EntityFactory::GetSingleton()->createEntity(teapotEntity);
	
	Ref<Entity> teapotChild = EntityFactory::GetSingleton()->createEntity(teapotEntity);
	teapotChild->getComponent<DiffuseVisualComponent>()->setTransform(Matrix::CreateTranslation({ 0.0f, 1.0f, 0.0f }));
	teapot->getComponent<HierarchyComponent>()->addChild(teapotChild);

	visualGraph->addChild(teapot);
	
	std::optional<int> ret = {};
	FrameTimer frameTimer;
	LoggingScopeTimer gameScopedLogger("GameTime");
	while (true)
	{
		frameTimer.reset();

		if (ret = window->processMessages())
			break;

		AudioSystem::GetSingleton()->update();

		static float x = 0;
		static float y = 0;
		static float u = 0;
		static float l = 0;
		static float roll = 0;
		static float pitch = 0;
		static float yaw = 0;
		if (GetAsyncKeyState(VK_LEFT))
		{
			u += 0.01;
		}
		if (GetAsyncKeyState(VK_RIGHT))
		{
			u += -0.01;
		}
		if (GetAsyncKeyState(VK_DOWN))
		{
			l += -0.01;
		}
		if (GetAsyncKeyState(VK_UP))
		{
			l += 0.01;
		}
		x -= l;
		y += u;

		if (GetAsyncKeyState(VK_NUMPAD7))
		{
			roll += 0.01;
		}
		if (GetAsyncKeyState(VK_NUMPAD4))
		{
			roll += -0.01;
		}
		if (GetAsyncKeyState(VK_NUMPAD8))
		{
			pitch += 0.01;
		}
		if (GetAsyncKeyState(VK_NUMPAD5))
		{
			pitch += -0.01;
		}
		if (GetAsyncKeyState(VK_NUMPAD9))
		{
			yaw += 0.01;
		}
		if (GetAsyncKeyState(VK_NUMPAD6))
		{
			yaw += -0.01;
		}
		if (GetAsyncKeyState('R'))
		{
			u = l = roll = pitch = yaw = 0;
		}
		x = l;
		y = u;
		//WARN(std::to_string(InputManager::GetSingleton()->isPressed(InputAction::Jump)));
		WARN(std::to_string(InputManager::GetSingleton()->getFloat(InputAction::MouseX)));
		WARN(std::to_string(InputManager::GetSingleton()->getFloat(InputAction::MouseY)));
		teapot->getComponent<TransformComponent>()->setTransform(Matrix::CreateFromYawPitchRoll(yaw, pitch, roll) * Matrix::CreateTranslation(0, y, 0.0f) * Matrix::CreateScale(x));

		RenderSystem::GetSingleton()->render(visualGraph.get(), window.get());
		InputManager::GetSingleton()->update();
		EventManager::GetSingleton()->tick();

		//frameTimer.showFPS();
	}

	return ret.value();
}
