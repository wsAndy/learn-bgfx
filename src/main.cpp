//
// Created by admin on 2024/4/29.
//

#include <Cluster.h>

int main(std::int32_t, gsl::zstring []) {
    big2::App app;

    app.AddExtension<big2::DefaultQuitConditionAppExtension>();
#if BIG2_IMGUI_ENABLED
    app.AddExtension<big2::ImGuiAppExtension>();
#endif // BIG2_IMGUI_ENABLED
    app.AddExtension<Cluster>();

    app.CreateWindow("My App", {800, 600});
    app.Run();

    return 0;
}