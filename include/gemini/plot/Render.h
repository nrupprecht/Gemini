//
// Created by Nathaniel Rupprecht on 9/17/22.
//

#pragma once

namespace gemini::plot {

// Forward declare the
class Manager;

//! \brief Something that should be rendered on a plot, e.g. a lineplot, barplot, inset shape, etc.
//!
class Render {
 public:
  class Impl {
   public:
    NO_DISCARD virtual bool Validate() const = 0;
    virtual void RegisterWithManager(Manager& manager) = 0;
    virtual void WriteToCanvas(core::Canvas& plotting_canvas) const = 0;
    NO_DISCARD virtual std::shared_ptr<Impl> Copy() const = 0;
  };

  //! \brief Check whether the render is well formed.
  NO_DISCARD bool Validate() const { return impl_->Validate(); }

  void RegisterWithManager(Manager& manager) const {
    // Set manager.
    impl_->RegisterWithManager(manager);
  }

  //! \brief Render the object onto a canvas.
  void WriteToCanvas(core::Canvas& plotting_canvas) const { impl_->WriteToCanvas(plotting_canvas); };

  //! \brief Create a deep copy of the render.
  NO_DISCARD Render Copy() const { return Render(impl_->Copy()); }

  //! \brief Create a render from an implementation.
  explicit Render(std::shared_ptr<Impl> impl) : impl_(std::move(impl)) {}

 protected:

  template<typename Concrete_t>
  typename Concrete_t::Impl* impl() {
    return reinterpret_cast<typename Concrete_t::Impl*>(impl_.get());
  }

  template<typename Concrete_t>
  NO_DISCARD const typename Concrete_t::Impl* impl() const {
    return reinterpret_cast<typename Concrete_t::Impl*>(impl_.get());
  }

  //! \brief Implementation of the Render.
  std::shared_ptr<Impl> impl_;
};


} // namespace gemini::plot
