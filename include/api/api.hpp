#include <Geode/loader/Dispatch.hpp>
#include <Geode/Geode.hpp>


namespace nytelyte::wave_trail_draw_fix {
  using AddTrailPart = DispatchFilter<HardStreak*, matjson::Value>;

  using DrawTrailPartWrapper = ObjWrapper<std::function<
    void(std::vector<CCPoint>&, std::vector<CCPoint>&)
  >>;
  
  using ClearWrapper = ObjWrapper<std::function<void()>>;

  template<typename T>
  void registerDrawer(const char* type) {
    static_assert(requires {{
      T::create(std::declval<matjson::Value&>())
    } -> std::same_as<T*>;},
      "Type T must have a "
      "static T* create(matjson::Value) "
      "method."
    );
    static_assert(requires(T* drawer) {{
        drawer->init(std::declval<matjson::Value&>())
    } -> std::same_as<bool>;},
      "Type T must have a "
      "bool init(matjson::Value) "
      "method."
    );
    static_assert(requires(T* drawer) {{ drawer->drawTrailPart(
        std::declval<std::vector<CCPoint>&>(), 
        std::declval<std::vector<CCPoint>&>()
      ) } -> std::same_as<void>;},
      "Type T must have a "
      "void drawTrailPart(std::vector<CCPoint>&, std::vector<CCPoint>&) "
      "method."
    );
    static_assert(requires(T* drawer) {{ drawer->clear() } -> std::same_as<void>;},
      "Type T must have a "
      "void clear() "
      "method."
    );
    new EventListener(+[](HardStreak* parent, matjson::Value data){
      T* object = T::create(data);
      if (!object) return ListenerResult::Propagate;
      auto offsetFunction = DrawTrailPartWrapper::create(
        [object](std::vector<CCPoint>& offset1, std::vector<CCPoint>& offset2) {
          object->drawTrailPart(offset1, offset2);
        }
      );
      object->setUserObject(
        "nytelyte.wave_trail_draw_fix/draw-trail-part-function",
         offsetFunction
      );
      
      auto clearFunction = ClearWrapper::create([object]() {
        object->clear();
      });
      object->setUserObject(
        "nytelyte.wave_trail_draw_fix/clear-function",
        clearFunction
      );

      parent->addChild(object);
      return ListenerResult::Propagate;
    }, AddTrailPart(type));
  }
}
