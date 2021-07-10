
#include "core/platform.h"
#if AGS_PLATFORM_OS_WINDOWS
#define NOMINMAX
#include <windows.h>
#endif
#include "allegro/file.h"
#include "util/path.h"
#include "util/stdio_compat.h"

// TODO: implement proper portable path length
#ifndef MAX_PATH
#define MAX_PATH 512
#endif

namespace AGS
{
namespace Common
{

namespace Path
{

bool IsDirectory(const String &filename)
{
    // stat() does not like trailing slashes, remove them
    String fixed_path = MakePathNoSlash(filename);
    return ags_directory_exists(fixed_path.GetCStr()) != 0;
}

bool IsFile(const String &filename)
{
    return ags_file_exists(filename.GetCStr()) != 0;
}

bool IsFileOrDir(const String &filename)
{
    // stat() does not like trailing slashes, remove them
    String fixed_path = MakePathNoSlash(filename);
    return ags_path_exists(fixed_path.GetCStr()) != 0;
}

String GetParent(const String &path)
{
    const char *cstr = path.GetCStr();
    const char *ptr_end = cstr + path.GetLength();
    for (const char *ptr = ptr_end; ptr > cstr; --ptr)
    {
        if (*ptr == '/' || *ptr == PATH_ALT_SEPARATOR)
            return String(cstr, cstr - ptr);
    }
    return ".";
}

String GetFilename(const String &path)
{
    const char *cstr = path.GetCStr();
    const char *ptr_end = cstr + path.GetLength();
    for (const char *ptr = ptr_end; ptr > cstr; --ptr)
    {
        if (*ptr == '/' || *ptr == PATH_ALT_SEPARATOR)
            return String(ptr + 1);
    }
    return path;
}

String GetFileExtension(const String &path)
{
    const char *cstr = path.GetCStr();
    const char *ptr_end = cstr + path.GetLength();
    for (const char *ptr = ptr_end; ptr >= cstr; --ptr)
    {
        if (*ptr == '.') return String(ptr + 1);
        if (*ptr == '/' || *ptr == PATH_ALT_SEPARATOR) break;
    }
    return "";
}

int ComparePaths(const String &path1, const String &path2)
{
    // Make minimal absolute paths
    String fixed_path1 = MakeAbsolutePath(path1);
    String fixed_path2 = MakeAbsolutePath(path2);

#if AGS_PLATFORM_OS_WINDOWS
    // On Windows make sure both are represented as short names (at least until we support wide paths)
    fixed_path1 = GetPathInASCII(fixed_path1);
    fixed_path2 = GetPathInASCII(fixed_path2);
#endif

    fixed_path1.TrimRight('/');
    fixed_path2.TrimRight('/');

    int cmp_result =
#if defined AGS_CASE_SENSITIVE_FILESYSTEM
        fixed_path1.Compare(fixed_path2);
#else
        fixed_path1.CompareNoCase(fixed_path2);
#endif // AGS_CASE_SENSITIVE_FILESYSTEM
    return cmp_result;
}

String GetDirectoryPath(const String &path)
{
    if (IsDirectory(path))
        return path;

    String dir = path;
    FixupPath(dir);
    size_t slash_at = dir.FindCharReverse('/');
    if (slash_at != -1)
    {
        dir.ClipMid(slash_at + 1);
        return dir;
    }
    return "./";
}

bool IsSameOrSubDir(const String &parent, const String &path)
{
    char can_parent[MAX_PATH];
    char can_path[MAX_PATH];
    char relative[MAX_PATH];
    // canonicalize_filename treats "." as "./." (file in working dir)
    const char *use_parent = parent == "." ? "./" : parent.GetCStr();
    const char *use_path   = path   == "." ? "./" : path.GetCStr();
    canonicalize_filename(can_parent, use_parent, MAX_PATH);
    canonicalize_filename(can_path, use_path, MAX_PATH);
    const char *pstr = make_relative_filename(relative, can_parent, can_path, MAX_PATH);
    if (!pstr)
        return false;
    for (pstr = strstr(pstr, ".."); pstr && *pstr; pstr = strstr(pstr, ".."))
    {
        pstr += 2;
        if (*pstr == '/' || *pstr == '\\' || *pstr == 0)
            return false;
    }
    return true;
}

bool IsRelativePath(const String &path)
{
    return is_relative_filename(path.GetCStr()) != 0;
}

void FixupPath(String &path)
{
#if AGS_PLATFORM_OS_WINDOWS
    path.Replace('\\', '/'); // bring Windows path separators to uniform style
#endif
    path.MergeSequences('/');
}

String MakePathNoSlash(const String &path)
{
    String dir_path = path;
    FixupPath(dir_path);
#if AGS_PLATFORM_OS_WINDOWS
    // if the path is 'x:/' don't strip the slash
    if (path.GetLength() == 3 && path[1u] == ':')
        ;
    else
#endif
    // if the path is '/' don't strip the slash
    if (dir_path.GetLength() > 1)
        dir_path.TrimRight('/');
    return dir_path;
}

String MakeTrailingSlash(const String &path)
{
    if (path.GetLast() == '/' || path.GetLast() == '\\')
        return path;
    String dir_path = String::FromFormat("%s/", path.GetCStr());
    FixupPath(dir_path);
    return dir_path;
}

String MakeAbsolutePath(const String &path)
{
    if (path.IsEmpty())
    {
        return "";
    }
    // canonicalize_filename treats "." as "./." (file in working dir)
    String abs_path = path == "." ? "./" : path;
#if AGS_PLATFORM_OS_WINDOWS
    // NOTE: cannot use long path names in the engine, because it does not have unicode strings support
    //
    //char long_path_buffer[MAX_PATH];
    //if (GetLongPathNameA(path, long_path_buffer, MAX_PATH) > 0)
    //{
    //    abs_path = long_path_buffer;
    //}
#endif
    char buf[MAX_PATH];
    canonicalize_filename(buf, abs_path.GetCStr(), MAX_PATH);
    abs_path = buf;
    FixupPath(abs_path);
    return abs_path;
}

String MakeRelativePath(const String &base, const String &path)
{
    char can_parent[MAX_PATH];
    char can_path[MAX_PATH];
    char relative[MAX_PATH];
    // canonicalize_filename treats "." as "./." (file in working dir)
    const char *use_parent = base == "." ? "./" : base.GetCStr();
    const char *use_path = path == "." ? "./" : path.GetCStr(); // FIXME?
    canonicalize_filename(can_parent, use_parent, MAX_PATH);
    canonicalize_filename(can_path, use_path, MAX_PATH);
    String rel_path = make_relative_filename(relative, can_parent, can_path, MAX_PATH);
    FixupPath(rel_path);
    return rel_path;
}

String ConcatPaths(const String &parent, const String &child)
{
    if (parent.IsEmpty())
        return child;
    if (child.IsEmpty())
        return parent;
    String path = String::FromFormat("%s/%s", parent.GetCStr(), child.GetCStr());
    FixupPath(path);
    return path;
}

String MakePath(const String &parent, const String &filename)
{
    String path = String::FromFormat("%s/%s", parent.GetCStr(), filename.GetCStr());
    FixupPath(path);
    return path;
}

String MakePath(const String &parent, const String &filename, const String &ext)
{
    String path = String::FromFormat("%s/%s.%s", parent.GetCStr(), filename.GetCStr(), ext.GetCStr());
    FixupPath(path);
    return path;
}

std::vector<String> Split(const String &path)
{
    return path.Split('/');
}

String FixupSharedFilename(const String &filename)
{
    const char *illegal_chars = "\\/:?\"<>|*";
    String fixed_name = filename;
    for (size_t i = 0; i < filename.GetLength(); ++i)
    {
        if (filename[i] < ' ')
        {
            fixed_name.SetAt(i, '_');
        }
        else
        {
            for (const char *ch_ptr = illegal_chars; *ch_ptr; ++ch_ptr)
                if (filename[i] == *ch_ptr)
                    fixed_name.SetAt(i, '_');
        }
    }
    return fixed_name;
}

String GetPathInASCII(const String &path)
{
#if AGS_PLATFORM_OS_WINDOWS
    char ascii_buffer[MAX_PATH];
    if (GetShortPathNameA(path.GetCStr(), ascii_buffer, MAX_PATH) == 0)
        return "";
    return ascii_buffer;
#else
    // TODO: implement conversion for other platforms!
    return path;
#endif
}

#if AGS_PLATFORM_OS_WINDOWS
String WidePathNameToAnsi(LPCWSTR pathw)
{
    WCHAR short_path[MAX_PATH];
    char ascii_buffer[MAX_PATH];
    LPCWSTR arg_path = pathw;
    if (GetShortPathNameW(arg_path, short_path, MAX_PATH) == 0)
        return "";
    WideCharToMultiByte(CP_ACP, 0, short_path, -1, ascii_buffer, MAX_PATH, NULL, NULL);
    return ascii_buffer;
}
#endif

String GetCmdLinePathInASCII(const char *arg, int arg_index)
{
#if AGS_PLATFORM_OS_WINDOWS
    // Hack for Windows in case there are unicode chars in the path.
    // The normal argv[] array has ????? instead of the unicode chars
    // and fails, so instead we manually get the short file name, which
    // is always using ASCII chars.
    int wargc = 0;
    LPWSTR *wargv = CommandLineToArgvW(GetCommandLineW(), &wargc);
    if (wargv == nullptr)
        return "";
    String path;
    if (arg_index <= wargc)
        path = WidePathNameToAnsi(wargv[arg_index]);
    LocalFree(wargv);
    return path;
#else
    // TODO: implement conversion for other platforms!
    return arg;
#endif
}

} // namespace Path

} // namespace Common
} // namespace AGS
