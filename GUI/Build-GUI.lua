project "GUI"
language "C++"
cppdialect "C++20"
targetdir "Binaries/%{cfg.buildcfg}"
debugdir "Binaries/%{cfg.targetdir}"
staticruntime "off"
targetname "NesSpaceChecker"

files
{
  "./Source/**.h", "./Source/**.hpp", "./Source/**.cpp",
  "./Source/App/**.hpp", "./Source/App/**.cpp",
  "./Source/Core/**.hpp", "./Source/Core/**.cpp",
  "./Source/UI/**.hpp", "./Source/UI/**.cpp",

  "./fonts/**.h",

  "../External/SDL2/include/**.h",
  "../External/imgui/*.h", "../External/imgui/*.cpp",
  "../External/imgui/backends/**.h", "../External/imgui/backends/**.cpp",
  "../External/imgui/misc/cpp/**.h", "../External/imgui/misc/cpp/**.cpp",
  "../External/FileBrowser/**.h", "../External/FileBrowser/**.cpp",
  "../External/termcolor/*.hpp",

  "../External/stb/**.h", "../External/stb/**.cpp",
}

vpaths {
  ["source/*"] = {
    "./Source/**.hpp",
    "./Source/**.cpp",
  },
  ["source/App/*"] = {
    "./Source/App/**.hpp",
    "./Source/App/**.cpp",
  },
  ["source/Core/*"] = {
    "./Source/Core/**.hpp",
    "./Source/Core/**.cpp",
  },
  ["source/UI/*"] = {
    "./Source/UI/**.hpp",
    "./Source/UI/**.cpp",
  },
  ["SDL2"] = {
    "../External/SDL2/include/**.h",
  },
  ["ImGui"] = {
    "../External/imgui/**.h",
    "../External/imgui/**.cpp",
    "../External/imgui/backends/**.h",
    "../External/imgui/backends/**.cpp",
    "../External/imgui/misc/cpp/*.h",
    "../External/imgui/misc/cpp/*.cpp",
    "../External/FileBrowser/**.h",
    "../External/FileBrowser/**.cpp",
  },
  ["termcolor"] = {"../External/termcolor/*.hpp"},
  ["stb"] = {
    "../External/stb/**.h",
    "../External/stb/**.cpp",
  },
}

includedirs
{
  "./Source",
  "./fonts",

  "../External/imgui",
  "../External/imgui/backends",
  "../External/imgui/misc/cpp",
  "../External/FileBrowser",

  "../External/termcolor",

  "../External/stb",
}

links
{
}

targetdir("../Binaries/" .. OutputDir .. "/%{prj.name}")
objdir("../Binaries/Intermediates/" .. OutputDir .. "/%{prj.name}")

-- Windows / Linux / macOS

filter "configurations:Debug"
  kind "ConsoleApp"
  defines { "_DEBUG" }
  runtime "Debug"
  symbols "On"


filter "configurations:Release"
  kind "ConsoleApp"
  defines { "_RELEASE" }
  runtime "Release"
  optimize "On"
  symbols "On"

-- Windows

filter { "system:windows" }
  staticruntime "on"

filter { "system:windows", "platforms:x86" }
  linkoptions { "/SAFESEH:NO" } -- Image Has Safe Exception Handers: No

filter { "system:windows", "configurations:Dist" }
  kind "WindowedApp"
  defines { "_DIST" }
  runtime "Release"
  optimize "On"
  symbols "Off"
  targetdir("../Binaries/" .. OutputDir .. "/NesSpaceChecker")
  entrypoint "mainCRTStartup"

filter "system:windows"
  files { '../Windows/Resources/resources.rc', '**.ico' }
  vpaths { ["Resources"] = { "../Windows/Resources/*.rc", "../Windows/Resources/*.ico" } }
  systemversion "latest"
  defines {
    "_CRT_SECURE_NO_WARNINGS",
    "SDL_MAIN_HANDLED", -- to avoid SDL_main
  }
  includedirs {
    "../External/SDL2/include",
    "../External/libusb/include"
  }
  links {
    "winmm.lib",
    "setupapi.lib",
    "version.lib",
    -- "Imm32.lib",
    "opengl32",
  }
  prebuildcommands {
    "powershell -ExecutionPolicy Bypass -File increment-build.ps1"
  }

filter { "system:windows", "configurations:Debug", "platforms:x86" }
  links {
    "SDL2-staticd"
  }
  libdirs {
    "../External/SDL2/lib/x86-static-debug",
  }

filter { "system:windows", "configurations:Debug", "platforms:x86_64" }
  links {
    "SDL2-staticd",
  }
  libdirs {
    "../External/SDL2/lib/x64-static-debug",
    "../External/libusb/VS2022/MS64/static",
  }

filter { "system:windows", "configurations:Release or Dist", "platforms:x86" }
  links {
    "SDL2-static",
  }
  libdirs {
    "../External/SDL2/lib/x86-static-release",
    "../External/libusb/VS2022/MS32/static",
  }

filter { "system:windows", "configurations:Release or Dist", "platforms:x86_64" }
  links {
    "SDL2-static",
  }
  libdirs {
    "../External/SDL2/lib/x64-static-release",
  }

-- Linux

filter "system:linux"
  buildoptions { "`sdl2-config --cflags`" }
  linkoptions { "`sdl2-config --libs`" }
  links {
    -- "usb-1.0",
    "GL",
    -- "dl",
    -- "pthread",
    "SDL2"
  }
  prebuildcommands {
    "sh ./increment-build.sh"
  }

filter { "system:linux", "configurations:Dist" }
  kind "WindowedApp"
  defines { "_DIST" }
  runtime "Release"
  optimize "On"
  symbols "Off"
  targetdir("../Binaries/" .. OutputDir .. "/NesSpaceChecker")

-- macOS

filter "system:macosx"
  buildoptions { "`sdl2-config --cflags`" }
  linkoptions {
    "`sdl2-config --libs`",
    "-framework OpenGL",
    "-framework CoreFoundation"
  }
  links {
    "usb-1.0",
  }
  includedirs
  {
    "../macOS"
  }
  prebuildcommands {
    "sh ./increment-build.sh"
  }

filter { "system:macosx", "configurations:Dist" }
  kind "ConsoleApp"
  defines { "_DIST" }
  runtime "Release"
  optimize "On"
  symbols "Off"
  targetdir("../Binaries/" .. OutputDir .. "/NesSpaceChecker")
  postbuildcommands
  {
    "{RMDIR} \"%{cfg.targetdir}/../app/NesSpaceChecker.app\"",
    "{MKDIR} \"%{cfg.targetdir}/../app/NesSpaceChecker.app\"",
    "{MKDIR} \"%{cfg.targetdir}/../app/NesSpaceChecker.app/Contents\"",
    "{MKDIR} \"%{cfg.targetdir}/../app/NesSpaceChecker.app/Contents/MacOS\"",
    "{MKDIR} \"%{cfg.targetdir}/../app/NesSpaceChecker.app/Contents/Resources\"",
    "{COPY} \"../macOS/Info.plist\" \"%{cfg.targetdir}/../app/NesSpaceChecker.app/Contents\"",
    "{COPY} \"%{cfg.targetdir}/NesSpaceChecker\" \"%{cfg.targetdir}/../app/NesSpaceChecker.app/Contents/MacOS\"",
    "{COPY} \"../macOS/app-icon.png\" \"%{cfg.targetdir}/../app/NesSpaceChecker.app/Contents/Resources\"",
  }
