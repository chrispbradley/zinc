'''
Created on May 22, 2013

@author: hsorby
'''
import unittest

from opencmiss.zinc.context import Context
from opencmiss.zinc.glyph import Glyph
from opencmiss.zinc.field import Field
from opencmiss.zinc.sceneviewer import Sceneviewer
from opencmiss.zinc import status

class GraphicsTestCase(unittest.TestCase):


    def setUp(self):
        self.context = Context('graphicstest')
        root_region = self.context.getDefaultRegion()
        self.scene = root_region.getScene()


    def tearDown(self):
        del self.scene
        del self.context


    def testGraphicsCreation(self):
        graphics = self.scene.createGraphicsPoints()
        self.assertTrue(graphics.isValid())
        result = graphics.setDomainType(Field.DOMAIN_NODES)
        self.assertEqual(status.OK, result)
        attributes = graphics.getGraphicspointattributes()
        self.assertTrue(attributes.isValid())
        glyph_module = self.context.getGlyphmodule()
        glyph_module.defineStandardGlyphs()
        result = attributes.setGlyphType(Glyph.SPHERE)
        self.assertEqual(status.OK, result)
        result = attributes.setGlyphType(Glyph.TYPE_INVALID)
        self.assertEqual(status.ERROR_ARGUMENT, result)
        glyphType = attributes.getGlyphType()
        self.assertEqual(Glyph.SPHERE, glyphType)

    def testGraphicsPointsSetBaseSize(self):
        graphics = self.scene.createGraphicsPoints()
        self.assertTrue(graphics.isValid())
        attributes = graphics.getGraphicspointattributes()
        result = attributes.setBaseSize([1])
        self.assertEqual(status.OK, result)
        result = attributes.setBaseSize([6, 2, 0.3])
        self.assertEqual(status.OK, result)
        result = attributes.setBaseSize(3)
        self.assertEqual(status.OK, result)

    def testGraphicsPointsGetBaseSize(self):
        graphics = self.scene.createGraphicsPoints()
        self.assertTrue(graphics.isValid())
        attributes = graphics.getGraphicspointattributes()
        result = attributes.setBaseSize(5.4)
        self.assertEqual(status.OK, result)
        base_size = attributes.getBaseSize(1)
        self.assertEqual(status.OK, base_size[0])
        self.assertEqual(5.4, base_size[1])
        attributes.setBaseSize([4.8, 2.1, 7])
        base_size = attributes.getBaseSize(3)
        self.assertEqual([4.8, 2.1, 7], base_size[1])
        
    def testSceneviewerBackgroundColour(self):
        svm = self.context.getSceneviewermodule()
        sv = svm.createSceneviewer(Sceneviewer.BUFFERING_MODE_DOUBLE, Sceneviewer.STEREO_MODE_MONO)
        
        result = sv.setBackgroundColourComponentRGB(0.3, 0.8, 0.65)
        self.assertEqual(1, result)
        (result, rgb) = sv.getBackgroundColourRGB()
        self.assertEqual(1, result)
        self.assertEqual([0.3, 0.8, 0.65], rgb)
        
        result = sv.setBackgroundColourRGB([0.1, 0.9, 0.4])
        self.assertEqual(1, result)
        (result, rgb) = sv.getBackgroundColourRGB()
        self.assertEqual(1, result)
        self.assertEqual([0.1, 0.9, 0.4], rgb)
        
        self.assertRaises(TypeError, sv.setBackgroundColourRGB, [3.0, 2.0])
    
def suite():
    #import ImportTestCase
    tests = unittest.TestSuite()
    tests.addTests(unittest.TestLoader().loadTestsFromTestCase(GraphicsTestCase))
    return tests

if __name__ == '__main__':
    unittest.TextTestRunner().run(suite())
