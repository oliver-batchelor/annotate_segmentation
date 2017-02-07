module Main where

import qualified Graphics.UI.Qtah.Widgets.QWidget as QWidget
import System.Environment (getArgs)
import System.FilePath (takeFileName)


import Control.Monad (forM_, unless, when)
import Data.Bits ((.|.))
import Data.Functor (void)
import Data.IORef (IORef, newIORef, readIORef, writeIORef)
--import Foreign.Hoppy.Runtime (withScopedPtr)

import qualified Graphics.UI.Qtah.Widgets.QApplication as QApplication
import qualified Graphics.UI.Qtah.Widgets.QFileDialog as QFileDialog
import qualified Graphics.UI.Qtah.Widgets.QMainWindow as QMainWindow
import Graphics.UI.Qtah.Widgets.QMainWindow (QMainWindow)
import qualified Graphics.UI.Qtah.Core.QCoreApplication as QCoreApplication


main :: IO ()
main = do
  getArgs >>= QApplication.new
  mainWindow <- makeMainWindow

  QMainWindow.setWindowTitle mainWindow "Annotate images"
  QWidget.show mainWindow
  QCoreApplication.exec

makeMainWindow :: IO QMainWindow
makeMainWindow = do
  window <- QMainWindow.new
  QWidget.resizeRaw window 640 480

  return window
