#include<Windows.h>
#include<memory>
#include<chrono>
#include<format>

import Lumina;

namespace {
	std::string ToString(Lumina::Math::F32x4 const& vec_) {
		return std::format(
			"{} {} {} {}\n\n",
			vec_.Get(0), vec_.Get(1), vec_.Get(2), vec_.Get(3)
		);
	}
	std::string ToString(Lumina::Math::F32x4x4<> const& m_) {
		return std::format(
			"{} {} {} {}\n{} {} {} {}\n{} {} {} {}\n{} {} {} {}\n\n",
			m_[0].Get(0), m_[0].Get(1), m_[0].Get(2), m_[0].Get(3),
			m_[1].Get(0), m_[1].Get(1), m_[1].Get(2), m_[1].Get(3),
			m_[2].Get(0), m_[2].Get(1), m_[2].Get(2), m_[2].Get(3),
			m_[3].Get(0), m_[3].Get(1), m_[3].Get(2), m_[3].Get(3)
		);
	}
	std::string ToString(Lumina::Math::F32x4x4<Lumina::Math::COLUMN_MAJOR> const& m_) {
		return std::format(
			"{} {} {} {}\n{} {} {} {}\n{} {} {} {}\n{} {} {} {}\n\n",
			m_[0].Get(0), m_[0].Get(1), m_[0].Get(2), m_[0].Get(3),
			m_[1].Get(0), m_[1].Get(1), m_[1].Get(2), m_[1].Get(3),
			m_[2].Get(0), m_[2].Get(1), m_[2].Get(2), m_[2].Get(3),
			m_[3].Get(0), m_[3].Get(1), m_[3].Get(2), m_[3].Get(3)
		);
	}
}

Lumina::I32 WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, Lumina::I32) {
	auto& rndGen = Lumina::Math::Random::Generator();
	auto rndNum =
		[&] () -> Lumina::F32 {
			return { (rndGen() & 0xFFFU) * 0.5f };
		};

	Lumina::Math::F32x4x4 m0{};
	Lumina::Math::F32x4x4 m1{};
	Lumina::Math::F32x4x4 m2{};
	m0 = {
		1.0f, 2.0f, 3.0f, 4.0f,
		5.0f, 6.0f, 7.0f, 8.0f,
		9.0f, 10.0f, 11.0f, 12.0f,
		13.0f, 14.0f, 15.0f, 16.0f
	};
	m1 = {
		17.0f, 18.0f, 19.0f, 20.0f,
		21.0f, 22.0f, 23.0f, 24.0f,
		25.0f, 26.0f, 27.0f, 28.0f,
		29.0f, 30.0f, 31.0f, 32.0f
	};
	Lumina::Math::F32x4x4<Lumina::Math::ROW_MAJOR>::Multiply(m2, m0, m1);
	::OutputDebugStringA(ToString(m0).data());
	::OutputDebugStringA(ToString(m1).data());
	::OutputDebugStringA(ToString(m2).data());

	Lumina::Math::F32x4 v00{};
	Lumina::Math::F32x4 v0{ 1.0f, 2.0f, 3.0f, 4.0f };
	Lumina::Math::F32x4 v1{ 5.5f, 6.6f, 7.7f, 8.8f };

	[[maybe_unused]] auto d2 = Lumina::Math::F32x4::Dot<2>(v0, v1);
	[[maybe_unused]] auto d3 = Lumina::Math::F32x4::Dot<3>(v0, v1);
	[[maybe_unused]] auto d4 = Lumina::Math::F32x4::Dot<4>(v0, v1);

	/*::OutputDebugStringA(
		std::format("{} {} {}\n",
			Lumina::Math::F32x4::Dot<2>(v0, v1),
			Lumina::Math::F32x4::Dot<3>(v0, v1),
			Lumina::Math::F32x4::Dot<4>(v0, v1)
		).data()
	);*/

	std::chrono::steady_clock::time_point t0{};
	std::chrono::steady_clock::time_point t1{};
	t0 = std::chrono::steady_clock::now();
	Lumina::Math::Quaternion q0{ 0.5f, -1.0f, 2.0f, -0.5f };
	Lumina::Math::Quaternion q1{ 0.0f, 0.0f, 0.0f, 1.000001f };
	for (int i = 0; i < 10000000; ++i) {
		/*m1 = {
			rndNum(), 1.0f, 1.0f, 1.0f,
			1.0f, rndNum(), 1.0f, 1.0f,
			1.0f, 1.0f, rndNum(), 1.0f,
			1.0f, 1.0f, 1.0f, rndNum(),
		};
		Lumina::Math::F32x4x4<>::Multiply(m2, m0, m1);*/

		q0 = q0 * q1;
	}
	t1 = std::chrono::steady_clock::now();
	auto duration = t1 - t0;
	::OutputDebugStringA(std::format("{}\n", duration.count()).data());

	[[maybe_unused]] auto vv = Lumina::Math::F32x4{ 5.0f, -6.0f, 3.0f, 4.0f };
	vv = -vv;
	//vv.X(100.0f);
	//vv.Y(150.0f);
	//vv.Z(200.0f);
	//vv.W(250.0f);
	[[maybe_unused]] auto vvx = vv.Get(0);
	[[maybe_unused]] auto vvy = vv.Get(1);
	[[maybe_unused]] auto vvz = vv.Get(2);
	[[maybe_unused]] auto vvw = vv.Get(3);
	::OutputDebugStringA(ToString(vv).data());
	::OutputDebugStringA(std::format("{} {} {} {}\n", vvx, vvy, vvz, vvw).data());
	auto vv1 = vv * m0;
	::OutputDebugStringA(ToString(vv1).data());

	[[maybe_unused]] auto vvlen = vv.Norm<2>();
	vvlen = vv.Norm<3>();
	vvlen = vv.Norm<4>();
	[[maybe_unused]] auto vvn = vv.Unit<2>();
	vvn = vv.Unit<3>();
	vvn = vv.Unit<4>();

	using Mat4CM = Lumina::Math::F32x4x4<Lumina::Math::COLUMN_MAJOR>;
	Mat4CM mmm0{ m0.Transpose() };
	Mat4CM mmm1{ m1.Transpose() };	
	Mat4CM mmm2{ mmm1 * mmm0 };
	::OutputDebugStringA(ToString(mmm0).data());
	::OutputDebugStringA(ToString(mmm1).data());
	::OutputDebugStringA(ToString(mmm2).data());
	auto vv2 = mmm0 * vv;
	::OutputDebugStringA(ToString(vv2).data());

	Lumina::WString str{ u8"なんでや！阪神関係ないやろ！\n" };
	Lumina::WStringView strV{ str };
	::OutputDebugStringW(strV.Data());

	Lumina::String str2{ u"Unus pro omnibus, onmes pro uno\n" };
	::OutputDebugStringA(str2.Data());

	Lumina::StringLiteral strL{ u"소환사의 협곡에 오신것을 환영합니다\n" };
	::OutputDebugStringW(reinterpret_cast<Lumina::C16 const*>(strL.Data));

	auto context{ std::make_unique<Lumina::Context>() };
	context->Initialize();

	while (context->Run());

	context->Finalize();

	return 0;
}