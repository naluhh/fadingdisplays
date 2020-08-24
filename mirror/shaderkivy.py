import sys
import kivy
kivy.require('1.0.6')

from kivy.app import App
from kivy.uix.floatlayout import FloatLayout
from kivy.uix.label import Label
from kivy.core.window import Window
from kivy.factory import Factory
from kivy.graphics import RenderContext
from pygments.lexers import GLShaderLexer as GLShaderLexer
from kivy.properties import StringProperty, ObjectProperty
from kivy.clock import Clock
from kivy.compat import PY2
from kivy.uix.boxlayout import BoxLayout
from kivy.graphics import Color
from kivy.graphics import Rectangle
from kivy.uix.widget import Widget
from kivy.uix.gridlayout import GridLayout
from kivy.uix.videoplayer import VideoPlayer
from datetime import datetime
from datetime import timedelta
from kivy.clock import Clock
from kivy.lang import Builder

fs_header = '''
#ifdef GL_ES
    precision highp float;
#endif

/* Outputs from the vertex shader */
varying vec4 frag_color;
varying vec2 tex_coord0;

/* uniform texture samplers */
uniform sampler2D texture0;

/* custom one */
uniform vec2 resolution;
uniform float time;
'''

vs_header = '''
#ifdef GL_ES
    precision highp float;
#endif

/* Outputs to the fragment shader */
varying vec4 frag_color;
varying vec2 tex_coord0;

/* vertex attributes */
attribute vec2     vPosition;
attribute vec2     vTexCoords0;

/* uniform variables */
uniform mat4       modelview_mat;
uniform mat4       projection_mat;
uniform vec4       color;

void main (void) {
  frag_color = color;
  tex_coord0 = vTexCoords0;
  gl_Position = projection_mat * modelview_mat * vec4(vPosition.xy, 0.0, 1.0);
}
'''

class ShaderViewer(FloatLayout):
    def __init__(self, **kwargs):
        self.canvas = RenderContext()
        with self.canvas:
            Color(1., 1., 1., 1.)
            Rectangle(size=Window.size)
        super(ShaderViewer, self).__init__(**kwargs)
        Clock.schedule_interval(self.update_shader, 0)
        self.canvas.shader.fs = fs_header + open("fragment.fs").read()
        self.canvas.shader.vs = vs_header
        Window.bind(on_resize=self.on_window_resize)

    def on_window_resize(self, window, width, height):
        self.canvas.clear()
        with self.canvas:
            Rectangle(size=(width, height))
        
    def update_shader(self, *args):
        s = self.canvas
        s['projection_mat'] = Window.render_context['projection_mat']
        s['time'] = Clock.get_boottime()
        s['resolution'] = list(map(float, self.size))
        s.ask_update()
    
class WrappedLabel(Label):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.bind(width=lambda *x:self.setter('text_size')(self, (self.width, None)), texture_size=lambda *x: self.setter('height')(self, self.texture_size[1]))

class ShaderEditorApp(App):
    def build(self):
        kwargs = {}
#        self.root = FloatLayout()
#        self.videoPlayer = VideoPlayer(source='/Users/charlydelaroche/Downloads/bbb_sunflower_2160p_60fps_normal.mp4',  state='play', options={'allow_stretch': True, 'eos': 'loop'})
#        self.shaderRoot = ShaderViewer(**kwargs)
#        for child in [child for child in videoPlayer.children]:
#            if type(child) == kivy.uix.gridlayout.GridLayout:
#                videoPlayer.remove_widget(child)
#        self.root.add_widget(videoPlayer)
#        self.root.add_widget(shaderRoot)
#        self.root.size_hint = (1.0, 1.0)
#        self.shaderRoot.size_hint = (1.0, 1.0)
        ##Window.bind(on_resize=self.on_window_resize)
        #return self.root
        self.now = datetime.now()
        self.interface = FloatLayout()
        self.interface.size_hint = (1.0, 1.0)
        Clock.schedule_interval(self.update_clock, 1)
        self.l = Label(text=datetime.now().strftime('%H:%M'), font_size='20sp')
        self.l.size_hint = (1.0, 1.0)
        self.l.halign = 'left'
        self.l.valign = 'top'
        self.l.font_name = 'OpenSans-Semibold'
        self.interface.add_widget(self.l)
        
        self.l2 = Label(text="2nd", font_size='15sp')
        self.l2.halign = 'left'
        self.l2.valign = 'top'
        self.l2.font_name = 'OpenSans-Semibold'
#
        self.interface.add_widget(self.l2)

        return self.interface

    def update_clock(self, *args):
        self.l.text = datetime.now().strftime('%H:%M')
        self.l._label.refresh()
        self.l.pos = (0, -500)
        
        self.l2.text = "toto"
        self.l2._label.refresh()
        self.l2.pos = (0, -self.l._label.texture.size[1])
        self.l2.text_size = (self.interface.size[0], self.interface.size[1] - self.l._label.texture.size[1])
        
        print(self.l._label.texture.size[1])
        ##self.l.size = self.l.text_size
        ##self.l2.pos = (0, -self.l.texture_size[1])
        ##print(self.l.texture_size[1])
        ##self.l2.text_size = self.interface.size##(self.interface.size[0], self.interface.size[1] - self.l.texture_size[1])
    ##def on_window_resize(self, window, width, height):
        ##self.root.export_to_png("test2.png")
        

if __name__ == '__main__':
    ShaderEditorApp().run()
