#pragma once


// Undefine the ERROR macro so that it can be used as a Toast message type
#ifdef ERROR
#undef ERROR
#endif

// Specify no min/max macros to use GLM functions
#define NOMINMAX


#include <cmath>
#include <vector>
#include <string>
#include <optional>
#include <algorithm>
#include <unordered_map>

#include <imgui/imgui.h>
#include <iconfontcppheaders/IconsFontAwesome6.h>

#include <Engine/Utils/ImGuiUtils.hpp>


namespace GUI {
    struct Toast {
        enum class MsgType { SUCCESS, WARNING, ERROR, INFO };
        
        std::string title;
        std::string content;
        MsgType type;

        float duration;     // Toast visibility duration (seconds)
        float elapsedTime = 0.0f;
        bool expired = false;

        ImVec2 position = { NAN, NAN }; // Toast X-position (for hide/display animations) & Y-position (for shift-down animation when another toast has expired); depends on where the window border is
        float lerpToPosX = NAN;         // Toast position animation - linearly interpolate to X-position; depends on toast state (valid/expired)
    
        float height = 0.0f;            // Toast height (for animations); depends on content
    };


    class ToastManager {
    public:
        ToastManager() :
            m_toastDuration(5.0f),
            m_toastWidth(400.0f),
            m_toastGap(10.0f),
            m_toastMargin(25.0f),
            m_toastAnimSnappiness(10.0f)
        {};
        ~ToastManager() = default;


        void addToast(std::string title, std::string content, Toast::MsgType type, std::optional<float> duration = std::nullopt) {
            Toast toast{};
            toast.title = std::move(title);
            toast.content = std::move(content);
            toast.type = type;

            if (duration.has_value())
                toast.duration = duration.value();
            else
                toast.duration = m_toastDuration;

            m_toasts.emplace_back(toast);
        }


        void render() {
            if (m_toasts.empty())
                return;

            float deltaTime = ImGui::GetIO().DeltaTime;

            ImVec2 viewportSize = ImGui::GetMainViewport()->Size;
            ImVec2 nextPos = ImVec2(viewportSize.x - m_toastMargin, viewportSize.y - m_toastMargin);
            const float originalPosY = nextPos.y;

            for (int i = 0; i < static_cast<int>(m_toasts.size()); i++) {
                auto &toast = m_toasts[i];

                toast.elapsedTime += deltaTime;

                if (std::isnan(toast.position.x) && std::isnan(toast.position.y) && std::isnan(toast.lerpToPosX)) {
                    toast.position = { viewportSize.x, nextPos.y };
                    toast.lerpToPosX = nextPos.x;
                }


                // Remove expired toasts
                if (toast.elapsedTime >= toast.duration) {
                    if (!toast.expired) {
                        toast.expired = true;

                        toast.lerpToPosX = viewportSize.x + m_toastWidth;   // Set final toast position to outside viewport
                    }


                    if (toast.position.x == toast.lerpToPosX) {
                        // Linear interpolation is complete; remove toast
                        m_toasts.erase(m_toasts.begin() + i);
                        i--;

                        continue;
                    }
                }


                // Set position & size (Stacked from bottom-right)
                ImGui::SetNextWindowPos(toast.position, ImGuiCond_Always, ImVec2(1.0f, 1.0f));
                ImGui::SetNextWindowSize(ImVec2(m_toastWidth, 0.0f));


                // Animate toast position
                    // Lerp toast Y-position to [pos of previous toast] in case a toast below has expired
                toast.position = lerpPosition(toast.position, ImVec2(toast.position.x, nextPos.y));
                    // Lerp toast X-position to ([final toast pos] if opening; [window border] if closing/expired: lerp to outside the viewport ONLY if this toast is the bottom-most one)
                if ((toast.expired && toast.position.y >= originalPosY) || !toast.expired)
                    toast.position = lerpPosition(toast.position, ImVec2(toast.lerpToPosX, toast.position.y));


                const std::string windowID = std::string("##toasts") + std::to_string(i);

                if (ImGui::Begin(windowID.c_str(), nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoFocusOnAppearing)) {
                    toast.height = ImGui::GetWindowHeight();
                    
                    ImGui::AlignTextToFramePadding();

                    ImGui::Indent();
                    {
                        using enum Toast::MsgType;
                        switch (toast.type) {
                        case SUCCESS:
                            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 255, 0, 255));
                                ImGui::Text(ICON_FA_CHECK);
                            ImGui::PopStyleColor();
                            break;

                        case WARNING:
                            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 0, 255));
                                ImGui::Text(ICON_FA_TRIANGLE_EXCLAMATION);
                            ImGui::PopStyleColor();
                            break;

                        case ERROR:
                            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
                                ImGui::Text(ICON_FA_XMARK);
                            ImGui::PopStyleColor();
                            break;

                        case INFO:
                        default:
                            ImGui::Text(ICON_FA_INFO);
                            break;
                        }

                        ImGui::SameLine();

                        ImGuiUtils::BoldText(toast.title.c_str());
                    }
                    ImGui::Unindent();

                    ImGui::Separator();

                    ImGuiUtils::Padding();
                    ImGui::Indent();
                        ImGui::TextUnformatted(toast.content.c_str());  // TextUnformatted instead of Text to prevent accidental formatting of content that contains `%` characters
                    ImGui::Unindent();
                    ImGuiUtils::Padding();


                    // Progress bar
                    float progress = (glm::max)(0.0f, 1.0f - (toast.elapsedTime / toast.duration));
                    ImGui::ProgressBar(progress, ImVec2(-1, 2), "");


                    // Offset Y-position for next toast
                    nextPos.y -= toast.height + m_toastGap;


                    ImGui::End();
                }
            }
        }


        void setToastDuration(float duration)           { m_toastDuration = duration; }
        void setToastWidth(float width)                 { m_toastWidth = width; }
		void setToastGap(float gap)                     { m_toastGap = gap; }
		void setToastMargin(float margin)               { m_toastMargin = margin; }
		void setToastAnimSnappiness(float snappiness)   { m_toastAnimSnappiness = snappiness; }

    private:
        std::vector<Toast> m_toasts;

        float m_toastDuration;
        float m_toastWidth;
        float m_toastGap;
        float m_toastMargin;                // The bottom-most toast's margin relative to window border
        float m_toastAnimSnappiness;        // The "snappiness" of the toast's open/close translation animation (i.e., lerp speed multiplier)


        ImVec2 lerpPosition(const ImVec2 &fromPos, const ImVec2 &toPos) const {
            ImVec2 currentPos = fromPos;

            // X-position
            if (approxEquals(currentPos.x, toPos.x))
                // Basically `if (currentPos.x == toPos.x)`, but accounting for imprecision caused by imperfect linear interpolation
                currentPos.x = toPos.x;
            else
                currentPos.x += (toPos.x - currentPos.x) * ImGui::GetIO().DeltaTime * m_toastAnimSnappiness;


            // Y-position
            if (approxEquals(currentPos.y, toPos.y))
                currentPos.y = toPos.y;
            else
                currentPos.y += (toPos.y - currentPos.y) * ImGui::GetIO().DeltaTime * m_toastAnimSnappiness;
        

            // Clamp to [fromPos..toPos]
                // NOTE: std::min and std::max are in parentheses to prevent MSVC from interpreting min and max as preprocessor macros defined in windows.h
            currentPos.x = std::clamp(currentPos.x, (std::min)(fromPos.x, toPos.x), (std::max)(fromPos.x, toPos.x));
            currentPos.y = std::clamp(currentPos.y, (std::min)(fromPos.y, toPos.y), (std::max)(fromPos.y, toPos.y));

            return currentPos;
        }


        /* Compares two values with a lenient difference tolerance. */
        bool approxEquals(float valA, float valB, const float epsilon = 1.0f) const {
            assert(epsilon > 0.0f);
            return glm::abs(valA - valB) < epsilon;
        }
    };
}
