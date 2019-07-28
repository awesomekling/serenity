#include <AK/FileSystemPath.h>
#include <AK/Optional.h>
#include <LibCore/CUserInfo.h>
#include <LibGUI/GDialog.h>
#include <LibGUI/GTableView.h>

class GDirectoryModel;
class GLabel;

class GFilePicker final : public GDialog {
    C_OBJECT(GFilePicker)
public:
    enum class Mode {
        Open,
        Save
    };

    static Optional<String> get_open_filepath();
    static Optional<String> get_save_filepath(const String& file_extension);
    static bool file_exists(const StringView& path);

<<<<<<< HEAD
    GFilePicker(Mode type = Mode::Open, const String& file_extension = ".txt", const StringView& path = "/", CObject* parent = nullptr);
=======
    GFilePicker(Mode type = Mode::Open, const StringView& path = String(get_current_user_home_path()), CObject* parent = nullptr);
>>>>>>> GFilePicker: Quality of life improvements (#370)
    virtual ~GFilePicker() override;

    FileSystemPath selected_file() const { return m_selected_file; }

private:
    void set_preview(const FileSystemPath&);
    void clear_preview();

    static String ok_button_name(Mode mode)
    {
        switch (mode) {
        case Mode::Open:
            return "Open";
        case Mode::Save:
            return "Save";
        default:
            return "OK";
        }
    }

    GTableView* m_view { nullptr };
    NonnullRefPtr<GDirectoryModel> m_model;
    FileSystemPath m_selected_file;

    GLabel* m_preview_image_label { nullptr };
    GLabel* m_preview_name_label { nullptr };
    GLabel* m_preview_geometry_label { nullptr };
    Mode m_mode { Mode::Open };
    const String& m_file_extension;
};
