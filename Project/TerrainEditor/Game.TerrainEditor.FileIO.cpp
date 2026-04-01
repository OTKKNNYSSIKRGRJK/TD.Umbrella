module;

#include<wrl.h>
#include<ShObjIdl.h>
#include<propkey.h>

module Game.TerrainEditor;

import <string>;

import <fstream>;
import <filesystem>;

import nlohmann.json;

namespace Game {
	auto TerrainEditor::OpenFile() -> void {
		using Microsoft::WRL::ComPtr;

		ComPtr<IFileOpenDialog> dialog{ nullptr };
		HRESULT hr{};
		hr = ::CoCreateInstance(
			CLSID_FileOpenDialog,
			nullptr,
			CLSCTX_ALL,
			IID_IFileOpenDialog,
			reinterpret_cast<void**>(dialog.GetAddressOf())
		);
		if (SUCCEEDED(hr)) {
			COMDLG_FILTERSPEC filterSpecs[]{
				{ L"JavaScript Object Notation (*.json)", L"*.json" },
				{ L"All Files (*.*)", L"*.*" },
			};
			hr = dialog->SetFileTypes(ARRAYSIZE(filterSpecs), filterSpecs);
			if (SUCCEEDED(hr)) {
				hr = dialog->SetDefaultExtension(L"json");
			}

			dialog->SetTitle(L"Open File");
			hr = dialog->Show(nullptr);

			ComPtr<IShellItem> fileItem{ nullptr };
			hr = dialog->GetResult(fileItem.GetAddressOf());
			if (SUCCEEDED(hr)) {
				ComPtr<IShellItem2> fileItemProperties{ nullptr };
				fileItem->QueryInterface(fileItemProperties.GetAddressOf());

				wchar_t* fileItemPath{ nullptr };
				fileItemProperties->GetDisplayName(
					SIGDN_FILESYSPATH,
					&fileItemPath
				);

				std::ifstream fileInputStream{};
				fileInputStream.open(fileItemPath);
				if (fileInputStream.good()) {
					auto&& input{ nlohmann::json::parse(fileInputStream) };
					InputData<nlohmann::json>(input);
				}

				//::OutputDebugStringW(fileItemPath);
				::CoTaskMemFree(fileItemPath);

				/*wchar_t* fileItemName{ nullptr };
				fileItemProperties->GetString(
					PKEY_ItemNameDisplayWithoutExtension,
					&fileItemName
				);
				::CoTaskMemFree(fileItemName);*/
			}
		}
	}

	auto TerrainEditor::SaveFile() const -> void {
		using Microsoft::WRL::ComPtr;

		ComPtr<IFileSaveDialog> dialog{ nullptr };
		HRESULT hr{};
		hr = ::CoCreateInstance(
			CLSID_FileSaveDialog,
			nullptr,
			CLSCTX_ALL,
			IID_IFileSaveDialog,
			reinterpret_cast<void**>(dialog.GetAddressOf())
		);
		if (SUCCEEDED(hr)) {
			COMDLG_FILTERSPEC filterSpecs[]{
				{ L"JavaScript Object Notation (*.json)", L"*.json" },
				{ L"All Files (*.*)", L"*.*" },
			};
			hr = dialog->SetFileTypes(ARRAYSIZE(filterSpecs), filterSpecs);
			if (SUCCEEDED(hr)) {
				hr = dialog->SetDefaultExtension(L"json");
			}

			dialog->SetTitle(L"Save File");
			hr = dialog->Show(nullptr);

			ComPtr<IShellItem> fileItem{ nullptr };
			hr = dialog->GetResult(fileItem.GetAddressOf());
			if (SUCCEEDED(hr)) {
				ComPtr<IShellItem2> fileItemProperties{ nullptr };
				fileItem->QueryInterface(fileItemProperties.GetAddressOf());

				wchar_t* fileItemPath{ nullptr };
				fileItemProperties->GetDisplayName(
					SIGDN_FILESYSPATH,
					&fileItemPath
				);

				std::ofstream fileOutputStream{};
				fileOutputStream.open(fileItemPath);
				if (fileOutputStream.good()) {
					nlohmann::ordered_json output{};
					OutputData<nlohmann::ordered_json>(output);
					fileOutputStream << output.dump(2);
				}

				//::OutputDebugStringW(fileItemPath);
				::CoTaskMemFree(fileItemPath);

				/*wchar_t* fileItemName{ nullptr };
				fileItemProperties->GetString(
					PKEY_ItemNameDisplayWithoutExtension,
					&fileItemName
				);
				::CoTaskMemFree(fileItemName);*/
			}
		}
	}
}