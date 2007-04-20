/**********************************************************************
  Draw - Draw Tool for Avogadro

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

#include "draw.h"
#include <avogadro/primitives.h>
#include <avogadro/color.h>
#include <avogadro/glwidget.h>

#include <openbabel/obiter.h>

#include <QtGui>

using namespace std;
using namespace OpenBabel;
using namespace Avogadro;

Draw::Draw() : Tool(), m_beginAtom(NULL), m_endAtom(NULL), m_bond(NULL), m_element(6), m_bondOrder(1)
{
  m_activateAction->setIcon(QIcon(QString::fromUtf8(":/draw/draw.png")));

  m_comboElements = new QComboBox(m_settingsWidget);
  m_comboElements->addItem("Hydrogen (1)");
  m_comboElements->addItem("Helium (2)");
  m_comboElements->addItem("Lithium (3)");
  m_comboElements->addItem("Beryllium (4)");
  m_comboElements->addItem("Boron (5)");
  m_comboElements->addItem("Carbon (6)");
  m_comboElements->addItem("Nitrogen (7)");
  m_comboElements->addItem("Oxygen (8)");
  m_comboElements->addItem("Fluorine (9)");
  m_comboElements->addItem("Neon (10)");
  m_comboElements->addItem("Sodium (11)");
  m_comboElements->addItem("Magnesium (12)");
  m_comboElements->addItem("Aluminum (13)");
  m_comboElements->addItem("Silicon (14)");
  m_comboElements->addItem("Phosphorus (15)");
  m_comboElements->addItem("Sulfur (16)");
  m_comboElements->addItem("Chlorine (17)");
  m_comboElements->addItem("Argon (18)");
  m_comboElements->setCurrentIndex(5);

  m_comboBondOrder = new QComboBox(m_settingsWidget);
  m_comboBondOrder->addItem("Single");
  m_comboBondOrder->addItem("Double");
  m_comboBondOrder->addItem("Triple");

  m_layout = new QVBoxLayout();
  m_layout->addWidget(m_comboElements);
  m_layout->addWidget(m_comboBondOrder);
  m_settingsWidget->setLayout(m_layout);

  connect(m_comboElements, SIGNAL(currentIndexChanged(int)),
      this, SLOT(elementChanged(int)));

  connect(m_comboBondOrder, SIGNAL(currentIndexChanged(int)),
      this, SLOT(bondOrderChanged(int)));
}

Draw::~Draw()
{
  delete m_activateAction;
}

void Draw::initialize()
{

}

void Draw::cleanup() 
{

};

void Draw::elementChanged( int index )
{
  setElement(index + 1);
}

void Draw::setElement( int index )
{
  m_element = index;
}

int Draw::element() const
{
  return m_element;
}

void Draw::bondOrderChanged( int index )
{
  setBondOrder(index + 1);
}

void Draw::setBondOrder( int index )
{
  m_bondOrder = index;
}

int Draw::bondOrder() const
{
  return m_bondOrder;
}

void Draw::mousePress(Molecule *molecule, GLWidget *widget, const QMouseEvent *event)
{
//   Molecule *molecule = widget->getMolecule();
  _buttons = event->buttons();

  m_movedSinceButtonPressed = false;
  m_lastDraggingPosition = event->pos();
  m_initialDragginggPosition = event->pos();

  //! List of hits from a selection/pick
  m_hits = widget->hits(event->pos().x()-2, event->pos().y()-2, 5, 5);

  if(_buttons & Qt::LeftButton)
  {
    if(m_hits.size())
    {
      if(m_hits[0].type() == Primitive::AtomType)
      {
        m_beginAtom = (Atom *)molecule->GetAtom(m_hits[0].name());
      }
    }
    else
    {
      m_beginAtom = newAtom(widget, event->pos().x(), event->pos().y());
      widget->updateGeometry();
      m_beginAtom->update();
    }
  }

}

void Draw::mouseMove(Molecule *molecule, GLWidget *widget, const QMouseEvent *event)
{
//   Molecule *molecule = widget->getMolecule();

  if((_buttons & Qt::LeftButton) && m_beginAtom)
  {
    QList<GLHit> hits;
    hits = widget->hits(event->pos().x()-2, event->pos().y()-2, 5, 5);

    bool hitBeginAtom = false;
    Atom *existingAtom = NULL;
    if(hits.size())
    {
      // parser our hits.  we want to know
      // if we hit another existingAtom that is not
      // the m_endAtom which we created
      for(int i=0; i < hits.size() && !hitBeginAtom; i++)
      {
        if(hits[i].type() == Primitive::AtomType)
        {
          // hit the same atom either moved here from somewhere else
          // or were already here.
          if(hits[i].name() == m_beginAtom->GetIdx())
          {
            hitBeginAtom = true;
          }
          else if(!m_endAtom)
          {
            existingAtom = (Atom *)molecule->GetAtom(hits[i].name());
          }
          else
          {
            if(hits[i].name() != m_endAtom->GetIdx())
            {
              existingAtom = (Atom *)molecule->GetAtom(hits[i].name());
            }
          }
        }
      }
    }
    if(hitBeginAtom)
    {
      if(m_endAtom)
      {
        molecule->DeleteAtom(m_endAtom);
        widget->updateGeometry();
        m_bond = NULL;
        m_endAtom = NULL;
      }
      else if(m_bond)
      {
        Atom *oldAtom = (Atom *)m_bond->GetEndAtom();
        oldAtom->DeleteBond(m_bond);
        molecule->DeleteBond(m_bond);
        m_bond=NULL;
      }
    }
    else if(existingAtom)
    {
      Bond *existingBond = (Bond *)molecule->GetBond(m_beginAtom, existingAtom);
      if(m_endAtom && m_bond)
      {
        if(!existingBond) {
          m_endAtom->DeleteBond(m_bond);
          m_bond->SetEnd(existingAtom);
          existingAtom->AddBond(m_bond);
          m_bond->update();
        }

        molecule->DeleteAtom(m_endAtom);
        widget->updateGeometry();
        m_endAtom = NULL;

        if(existingBond) {
          m_bond = NULL;
        }
      }
      else if(!m_bond && !existingBond)
      {
        m_bond = newBond(molecule, m_beginAtom, existingAtom);
        m_bond->update();
      }
    }
    else // if(!existingAtom && !hitBeginAtom)
    {
      if(!m_endAtom)
      {
        m_endAtom = newAtom(widget, event->pos().x(), event->pos().y());
        widget->updateGeometry();
        if(!m_bond)
        {
          m_bond = newBond(molecule, m_beginAtom, m_endAtom);
        }
        else
        {
          Atom *oldAtom = (Atom *)m_bond->GetEndAtom();
          oldAtom->DeleteBond(m_bond);
          m_bond->SetEnd(m_endAtom);
          m_endAtom->AddBond(m_bond);
        }
        m_bond->update();
        m_endAtom->update();
      }
      else
      {
        m_endAtom->setPos(unProject(widget, event->pos().x(), event->pos().y()));
        m_endAtom->update();
      }
    }
  }

}

void Draw::mouseRelease(Molecule *molecule, GLWidget *widget, const QMouseEvent *event)
{
  if(_buttons & Qt::LeftButton)
  {
    m_beginAtom=NULL;
    m_bond=NULL;
    m_endAtom=NULL;

    // create the undo action for creating endAtom and bond
    //  pass along atom idx, element, vector, bond idx, order, start/end
  }
  else if(_buttons & Qt::RightButton)
  {
    QList<GLHit> hits;
    hits = widget->hits(event->pos().x()-2, event->pos().y()-2, 5, 5);
    if(hits.size())
    {
      // get our top hit
      if(hits[0].type() == Primitive::AtomType)
      {
        Atom *atom = (Atom *)molecule->GetAtom(hits[0].name());
        molecule->DeleteAtom(atom);
        widget->updateGeometry();
      }
    }
  }
}

void Draw::wheel(Molecule *molecule, GLWidget *widget, const QWheelEvent *event)
{
}

Eigen::Vector3d Draw::unProject(GLWidget *widget, int x, int y)
{
  // retrieve the 3D coords of the center of the molecule
  Eigen::Vector3d center = widget->center();

  // project center
  Eigen::Vector3d projectedCenter = widget->project(center.x(), center.y(), center.z());

  // now projectedCenter.z() gives us the Z-index of the center of the molecule.
  // this is all what we need to know - we don't care about the x() and y() coords of
  // projectedCenter.

  // Now unproject the pixel of coordinates (x,height-y) into a 3D point having the same Z-index
  // as the molecule's center.
  Eigen::Vector3d pos = widget->unProject(x, y, projectedCenter.z());

  return pos;
}

Atom *Draw::newAtom(GLWidget *widget, int x, int y)
{
  // GRH (for reasons I don't understand, calling Begin/EndModify here
  // causes crashes with multiple bond orders
  // (need to investigate, probable OB bug.

  widget->molecule()->BeginModify();
  Atom *atom = static_cast<Atom*>(widget->molecule()->NewAtom());
  atom->setPos(unProject(widget, x, y));
  atom->SetAtomicNum(element());
  widget->molecule()->EndModify();
  
  return atom;
}

Bond *Draw::newBond(Molecule *molecule, Atom *beginAtom, Atom *endAtom)
{
  molecule->BeginModify();
  Bond *bond = (Bond *)molecule->NewBond();
  bond->SetBO(bondOrder());
  bond->SetBegin(beginAtom);
  bond->SetEnd(endAtom);
  beginAtom->AddBond(bond);
  endAtom->AddBond(bond);
  molecule->EndModify();

  return bond;
}

#include "draw.moc"
Q_EXPORT_PLUGIN2(draw, Draw)
