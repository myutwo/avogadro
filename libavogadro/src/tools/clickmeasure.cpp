/**********************************************************************
  ClickMeasure - ClickMeasure Tool for Avogadro

  Copyright (C) 2006 by Geoffrey R. Hutchison
  Some portions Copyright (C) 2006 by Donald E. Curtis

  This file is part of the Avogadro molecular editor project.
  For more information, see <http://avogadro.sourceforge.net/>

  Some code is based on Open Babel
  For more information, see <http://openbabel.sourceforge.net/>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#include "clickmeasure.h"
#include <avogadro/primitives.h>
#include <avogadro/color.h>
#include <avogadro/glwidget.h>

#include <openbabel/obiter.h>
#include <eigen/regression.h>

#include <math.h>

#include <QtGui>
#include <QDebug>

using namespace std;
using namespace OpenBabel;
using namespace Avogadro;

ClickMeasure::ClickMeasure() : Tool(),  m_numSelectedAtoms(0), m_dl(0), m_line(new Cylinder(0))
{
  m_activateAction->setIcon(QIcon(QString::fromUtf8(":/measure/measure.png")));

  // clear the selected atoms
  int size = m_selectedAtoms.size();
  for(int i=0; i<size; i++)
  {
    m_selectedAtoms[i] = NULL;
  }
}

ClickMeasure::~ClickMeasure()
{
  if(m_dl)
  {
    glDeleteLists(m_dl,1);
  }

  delete m_line;
  delete m_activateAction;
}

void ClickMeasure::initialize()
{
}

void ClickMeasure::cleanup()
{
}

void ClickMeasure::mousePress(Molecule *molecule, GLWidget *widget, const QMouseEvent *event)
{
  qDebug() << event->pos();
  //! List of hits from initial click
  m_hits = widget->hits(event->pos().x()-2, event->pos().y()-2, 5, 5);

  if(m_hits.size() && event->buttons() & Qt::LeftButton)
  {
    qDebug() << "Here";
    Atom *atom = (Atom *)molecule->GetAtom(m_hits[0].name());
    if(m_numSelectedAtoms < 1) {
      m_selectedAtoms[m_numSelectedAtoms++] = atom;
    }
    else if(m_numSelectedAtoms < 3) {
      m_selectedAtoms[m_numSelectedAtoms++] = atom;

      // generate the display list if needed
      if(!m_dl) {
        m_dl = glGenLists(1);
      }

      // get GL Coordinates for text
      glPushMatrix();
      glLoadIdentity();
      Eigen::Vector3d labelPos = widget->unProject(5, widget->height()-5, 0.1);
      Eigen::Vector3d distancePos[2];
      distancePos[0] = widget->unProject(90, widget->height()-5, 0.1);
      distancePos[1] = widget->unProject(150, widget->height()-5, 0.1);
      Eigen::Vector3d anglePos = widget->unProject(50, widget->height()-25, 0.1);
      Eigen::Vector3d angleLabelPos = widget->unProject(5, widget->height()-25, 0.1);
      glPopMatrix();

      glNewList(m_dl, GL_COMPILE);
      glMatrixMode(GL_MODELVIEW);
      glPushMatrix();
      glPushAttrib(GL_LIGHTING_BIT);
      glDisable(GL_LIGHTING);

      Eigen::Vector3d vector[2];
      vector[0] = m_selectedAtoms[0]->pos() - m_selectedAtoms[1]->pos();
      double distance = vector[0].norm();
      double angle;

//       Color(1.0,0.0,0.0,1.0).applyAsMaterials();
      glColor3f(1.0,0.0,0.0);
      Eigen::Vector3d pos = m_selectedAtoms[0]->pos();
      float radius = 0.18 + etab.GetVdwRad(m_selectedAtoms[0]->GetAtomicNum()) * 0.3;
      widget->renderText(pos.x() + radius, pos.y(), pos.z() + radius, tr("*0"));

//       Color(0.0,1.0,0.0,1.0).applyAsMaterials();
      glColor3f(0.0,1.0,0.0);
      pos = m_selectedAtoms[1]->pos();
      radius = 0.18 + etab.GetVdwRad(m_selectedAtoms[1]->GetAtomicNum()) * 0.3;
      widget->renderText(pos.x() + radius, pos.y(), pos.z() + radius, tr("*1"));

      if(m_numSelectedAtoms == 3)
      {
        vector[1] = m_selectedAtoms[2]->pos() - m_selectedAtoms[1]->pos();

        Eigen::Vector3d normalizedVectors[2];
        normalizedVectors[0] = vector[0];
        normalizedVectors[0].normalize();
        normalizedVectors[1] = vector[1];
        normalizedVectors[1].normalize();
        
        angle = acos(normalizedVectors[0].dot(normalizedVectors[1])) * 180/M_PI;
//         angleString = tr("Angle: ") + QString::number(angle);
        pos = m_selectedAtoms[2]->pos();
        radius = 0.18 + etab.GetVdwRad(m_selectedAtoms[2]->GetAtomicNum()) * 0.3;
//         Color(0.0,0.0,1.0,1.0).applyAsMaterials();
        glColor3f(0.0,0.0,1.0);
        widget->renderText(pos.x() + radius, pos.y(), pos.z() + radius, tr("*2"));
      }
      glLoadIdentity();
//       Color(1.0,1.0,1.0,1.0).applyAsMaterials();
      glColor3f(1.0,1.0,1.0);
      widget->renderText(labelPos.x(), labelPos.y(), labelPos.z(), tr("Distance(s):"));
      if(m_numSelectedAtoms == 3) {
        glColor3f(1.0,1.0,1.0);
//         Color(1.0,1.0,1.0,1.0).applyAsMaterials();
        widget->renderText(angleLabelPos.x(), angleLabelPos.y(), angleLabelPos.z(), QString("Angle:"));
        glColor3f(0.8, 0.8, 0.8);
//         Color(0.8,0.8,0.8,1.0).applyAsMaterials();
        widget->renderText(anglePos.x(), anglePos.y(), anglePos.z(), QString::number(angle));
//         Color(0.0,1.0,1.0,1.0).applyAsMaterials();
        glColor3f(0.0,1.0,1.0);
        widget->renderText(distancePos[1].x(), distancePos[1].y(), distancePos[1].z(), QString::number(vector[1].norm()));
      }
//       Color(1.0,1.0,0.0,1.0).applyAsMaterials();
      glColor3f(1.0,1.0,0.0);
      widget->renderText(distancePos[0].x(), distancePos[0].y(), distancePos[0].z(), QString::number(vector[0].norm()));

      glPopAttrib();
      glPopMatrix();
      glEndList();

      widget->addDL(m_dl);
      widget->updateGL();
    }
    else if(m_numSelectedAtoms < 3) {
      
    }
  }
  else
  {
    qDebug() << "Reset";
    int size = m_selectedAtoms.size();
    for(int i=0; i<size; i++)
    {
      m_selectedAtoms[i] = NULL;
    }
    m_numSelectedAtoms = 0;
    widget->removeDL(m_dl);
    widget->updateGL();
    return;
  }
}

void ClickMeasure::mouseMove(Molecule *molecule, GLWidget *widget, const QMouseEvent *event)
{
}

void ClickMeasure::mouseRelease(Molecule *molecule, GLWidget *widget, const QMouseEvent *event)
{
}

void ClickMeasure::wheel(Molecule *molecule, GLWidget *widget, const QWheelEvent *event)
{
}

#include "clickmeasure.moc"
Q_EXPORT_PLUGIN2(clickmeasure, ClickMeasure)
