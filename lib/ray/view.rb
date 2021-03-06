module Ray
  class View
    def ==(other)
      other.is_a?(Ray::View) && self.matrix == other.matrix
    end

    # Increases zoom (making object appear bigger). The center of the view and
    # its viewport aren't affected by this.
    #
    # @param [Float, Ray::Vector2] value Zoom coefficient
    # @see unzoom_by
    def zoom_by(value)
      self.size /= value
    end

    # Decreases zoom (making object appear smaller).
    #
    # @param [Float, Ray::Vector2] value Zoom coefficient
    # @see zoom_by
    def unzoom_by(value)
      self.size *= value
    end

    # @return [Ray::Rect] Rect representing the part of the world that can be
    #   seen through this view.
    def rect
      size = self.size
      pos  = self.center

      Ray::Rect[pos.x - size.x / 2, pos.y - size.y / 2, size.x, size.y]
    end

    def x; center.x; end
    def y; center.y; end
    def w; size.w;   end
    def h; size.h;   end

    def x=(val); self.center = [val, y]; end
    def y=(val); self.center = [x, val]; end
    def w=(val); self.size   = [val, h]; end
    def h=(val); self.size   = [w, val]; end

    def inspect
      "#<#{self.class} center=#{center} size=#{size} viewport=#{viewport}>"
    end
  end
end
